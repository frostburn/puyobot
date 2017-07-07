from time import sleep
import struct
import subprocess
import argparse
import requests

NUM_FLOORS = 2
WIDTH = 6
HEIGHT = 10
NUM_COLORS = 6
GARBAGE = 5


class Deal(object):
    fstring = "B"

    def __init__(self, blocks):
        self.blocks = blocks

    @classmethod
    def unpack(cls, string):
        (content, ) = struct.unpack(cls.fstring, string)
        return cls([(content & 15) + 1, (content >> 4) + 1])

    def pack(self):
        return struct.pack(self.fstring, (self.blocks[0] - 1) | ((self.blocks[1] - 1) << 4))


class State(object):
    fstring = "Q" * NUM_FLOORS * NUM_COLORS

    def __init__(self, blocks):
        self.blocks = [0] * (WIDTH * HEIGHT * NUM_FLOORS - len(blocks)) + blocks

    def pprint(self):
        blocks = self.blocks[:]
        for i in range(0, len(blocks), WIDTH):
            print (blocks[i: i + WIDTH])

    @classmethod
    def unpack(cls, string):
        floors = struct.unpack(cls.fstring, string)
        blocks = [0] * NUM_FLOORS * WIDTH * HEIGHT

        for j in range(NUM_FLOORS):
            for i in range(WIDTH * HEIGHT):
                for k in range(NUM_COLORS):
                    p = 1 << i
                    index = k + j * NUM_COLORS
                    if p & floors[index]:
                        index = i + WIDTH * HEIGHT * j
                        if k == GARBAGE:
                            color = -1
                        else:
                            color = k + 1
                        blocks[index] = color
        return cls(blocks)

    def pack(self):
        floors = [0] * NUM_FLOORS * NUM_COLORS
        for i, block in enumerate(self.blocks):
            index = i % (WIDTH * HEIGHT)
            floor = i // (WIDTH * HEIGHT)
            if block:
                if block > 0:
                    color = block - 1
                else:
                    color = GARBAGE
                floors[color + floor * NUM_COLORS] |= 1 << index
        return struct.pack(self.fstring, *floors)


class Player(object):
    _fstring = "iiiiiiiiii"
    fstring = State.fstring + _fstring

    def __init__(self, json=None):
        if not json:
            return
        self.state = State(json['blocks'])
        self.deal_index = json['dealIndex']
        self.chain = json['chainNumber']
        self.chain_score = json['chainScore']
        self.total_score = json['totalScore']
        self.chain_all_clear_bonus = json['chainAllClearBonus']
        self.all_clear_bonus = json['allClearBonus']
        self.game_overs = json['gameOvers']
        self.pending_nuisance = json['pendingNuisance']
        self.leftover_score = json['leftoverScore']
        self.nuisance_x = json['nuisanceX']

    @classmethod
    def unpack(cls, string):
        size = struct.calcsize(State.fstring)
        string, state_string = string[size:], string[:size]
        state = State.unpack(state_string)
        params = struct.unpack(cls._fstring, string)

        instance = cls()
        instance.state = state
        instance.deal_index = params[0]
        instance.chain = params[1]
        instance.chain_score = params[2]
        instance.total_score = params[3]
        instance.chain_all_clear_bonus = params[4]
        instance.all_clear_bonus = params[5]
        instance.game_overs = params[6]
        instance.pending_nuisance = params[7]
        instance.leftover_score = params[8]
        instance.nuisance_x = params[9]
        return instance

    def pack(self):
        string = self.state.pack()
        string += struct.pack(
            self._fstring,
            self.deal_index,
            self.chain,
            self.chain_score,
            self.total_score,
            self.chain_all_clear_bonus,
            self.all_clear_bonus,
            self.game_overs,
            self.pending_nuisance,
            self.leftover_score,
            self.nuisance_x,
        )
        return string

class Game(object):
    fstring = "iPiiP"

    def __init__(self, json=None):
        if not json:
            return
        self.num_deals = json['numDeals']
        self.deals = [Deal(d) for d in json['deals']]
        self.players = [Player(p) for p in json['childStates']]

    @classmethod
    def unpack(cls, string):
        size = struct.calcsize(cls.fstring)
        string, game_string = string[size:], string[:size]
        num_players, _, num_deals, total_num_deals, _ = struct.unpack(cls.fstring, game_string)
        players = []
        for _ in range(num_players):
            size = struct.calcsize(Player.fstring)
            string, player_string = string[size:], string[:size]
            players.append(Player.unpack(player_string))
        deals = []
        for _ in range(total_num_deals):
            size = struct.calcsize(Deal.fstring)
            string, deals_string = string[size:], string[:size]
            deals.append(Deal.unpack(deals_string))

        instance = cls()
        instance.players = players
        instance.num_deals = num_deals
        instance.deals = deals
        assert (not string)
        return instance

    def pack(self):
        string = struct.pack(self.fstring, len(self.players), 0, self.num_deals, len(self.deals), 0)
        for player in self.players:
            string += player.pack()
        for deal in self.deals:
            string += deal.pack()
        return string


def main(command, url, autojoin=False):
    mode = 'puyo:duel'
    if url.endswith('/'):
        url = url[:-1]
    restart = False
    while True:
        if restart:
            sleep(1)
        response = requests.get('{}/game/list?status=open&mode={}'.format(url, mode))
        payload = {
            'metadata': {'name': 'frostbot'},
        }
        if (response.json()['games']) and autojoin:
            payload['id'] = response.json()['games'][0]['id']
            response = requests.post('{}/game/join'.format(url), json=payload)
        else:
            payload['mode'] = mode
            response = requests.post('{}/game/create/'.format(url), json=payload)
        print (response.content)
        uuid = response.json()['id']
        restart = False
        try:
            while not restart:
                sleep(0.2)
                response = requests.get('{}/play/{}?poll=1'.format(url, uuid))
                state = response.json()
                status = state.get('status', {})
                if status.get('terminated'):
                    print (status.get('result'), 'restarting...')
                    restart = True
                    break
                if state.get('canPlay'):
                    time = state['time']
                    player = state['player']
                    game = Game(state)
                    deal = game.deals[game.players[player].deal_index]
                    # TODO: Use sockets instead of files to communicate.
                    with open("in.bin", "wb") as f:
                        f.write(game.pack())
                    subprocess.call([command, str(player)])
                    with open("out.bin", "rb") as f:
                        game = Game.unpack(f.read())
                    blocks = game.players[player].state.blocks[:2 * WIDTH]

                    event = {
                        'type': 'addPuyos',
                        'blocks': blocks,
                    }
                    response = requests.post('{}/play/{}'.format(url, uuid), json=event)
                    if not response.json()['success']:
                        # The bots pick badly sometimes so we need to suicide like this
                        for i in range(WIDTH - 1):
                            suicide = ([0] * i) + deal.blocks + ([0] * (WIDTH - i - 2))
                            print (suicide)
                            event = {
                                'type': 'addPuyos',
                                'blocks': suicide,
                            }
                            response = requests.post('{}/play/{}'.format(url, uuid), json=event)
                            if response.json()['success']:
                                break
                    if not response.json()['success']:
                        reason = response.json().get('reason', '')
                        raise ValueError('Cannot play a move because %s' % reason)
        finally:
            response = requests.delete('{}/play/{}'.format(url, uuid))
            print (response.content)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Connection layer between a HTTP API and a binary socket')
    parser.add_argument('command', metavar='command', type=str, help='Executable that calculates the next move')
    parser.add_argument('url', metavar='url', type=str, help='API URL')
    parser.add_argument('--autojoin', action='store_true')

    args = parser.parse_args()
    main(args.command, args.url, args.autojoin)

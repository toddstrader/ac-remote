#!/usr/bin/env python3

import json
import cmd
import sys
import socket

# TODO - make this not hard coded
thing_host = "attic_thing"
# TODO - share magic number with Arduino code?
thing_port = 4178
# Magic number from https://github.com/mattjm/Fujitsu_IR/blob/master/lircd.conf
ir_freq = 38
# TODO - command line argument
json_codes = "ac-codes.json"

class AcCmd(cmd.Cmd):
    intro = 'Fujitsu Halcyon remote (help or ? for a list of commands)\n'
    prompt = '(Halcyon) '

    fan_speeds = ['auto', 'quiet', 'low', 'medium', 'high']

    with open(json_codes, 'r') as f:
        codes = json.load(f)

    def do_dry_on(self, rest=None):
        'Turn on the dry mode (auto fan, 88F)'
        self.send(self.codes['dry-on']['data'])

    def do_cool_on(self, rest=None):
        'Turn on the cooling mode (auto fan, 88F)'
        self.send(self.codes['cool-on']['data'])

    def do_fan_on(self, rest=None):
        'Turn on the fan mode (auto fan)'
        self.send(self.codes['fan-on']['data'])

    def do_min_heat(self, rest=None):
        'Turn on the minimum heat mode (50F)'
        self.send(self.codes['min-heat']['data'])

    def do_dry(self, args):
        'Set the dry temperature and fan speed, parameters = (FAN TEMP)'
        params = self.parse_fan_and_temp(args)
        if params['TEMP'] < 64 or params['TEMP'] > 88:
            raise RuntimeError("dry temperature must be between 64 - 88 F")
        self.send(self.codes['dry-{}-{}F'.format(params['FAN'], params['TEMP'])]['data'])

    def do_cool(self, args):
        'Set the cool temperature and fan speed, parameters = (FAN TEMP)'
        params = self.parse_fan_and_temp(args)
        if params['TEMP'] < 64 or params['TEMP'] > 88:
            raise RuntimeError("cool temperature must be between 64 - 88 F")
        self.send(self.codes['cool-{}-{}F'.format(params['FAN'], params['TEMP'])]['data'])

    def do_fan(self, args):
        'Set the fan fan speed, parameters = (FAN)'
        fan = self.parse_fan(args)
        # fan temp doesn't matter, but the IR codes still send a temp and this is what it was captured with
        self.send(self.codes['fan-{}-64F'.format(fan)]['data'])

    def do_heat(self, args):
        'Set the heat temperature and fan speed, parameters = (FAN TEMP)'
        params = self.parse_fan_and_temp(args)
        if params['TEMP'] < 60 or params['TEMP'] > 76:
            raise RuntimeError("heat temperature must be between 60 - 76 F")
        self.send(self.codes['heat-{}-{}F'.format(params['FAN'], params['TEMP'])]['data'])

    def do_off(self, rest=None):
        'Turn the unit off'
        self.send(self.codes['turn-off']['data'])

    def do_exit(self, rest=None):
        'Exit the utility'
        sys.exit()

    def do_read_temp(self, rest=None):
        'Read the temperature sensor'
        buff = self.send([])
        val = int(buff.decode("ascii"))
        # ADC range is 0-1023, conver to millivolts
        mv = 1000*val/1024.
        # TMP36 has a voltage offset of 0.5V and 10mV/C
        tempC = (mv - 500) / 10
        print("{0:.1f} C".format(tempC))
        tempF = tempC * 9/5 + 32
        print("{0:.1f} F".format(tempF))

    def send(self, data):
        params = "{},{}".format(ir_freq, len(data))
        for value in data:
            params += ",{}".format(value)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
#TODO -- wrap in a try here
            sock.connect((thing_host, thing_port))
            sock.sendall(bytearray(params, 'ascii'))
            buff = sock.recv(256)
#TODO -- receive a response
            sock.close()
            return buff

    def parse_fan_and_temp(self, args):
        toks = args.split()
        if len(toks) != 2:
            raise RuntimeError("require 'FAN TEMP' parameters")

        fan = self.parse_fan(toks[0])

        # Only even temps (in F) are allowed by the Halcyon
        temp = int(int(toks[1])/2)*2

        result = {'FAN': fan, 'TEMP': temp}
        return result

    def parse_fan(self, arg):
        fan = arg.lower()
        if fan not in self.fan_speeds:
            raise RuntimeError("unknown fan speed, need one of ({})".format(" ".join(self.fan_speeds)))
        return fan


AcCmd().cmdloop()

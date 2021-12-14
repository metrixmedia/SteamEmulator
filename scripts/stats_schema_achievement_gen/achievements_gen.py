import vdf
import sys
import os
import json


if len(sys.argv) < 2:
    print("format: {} UserGameStatsSchema_480.bin".format(sys.argv[0]))
    exit(0)


with open(sys.argv[1], 'rb') as f:
    schema = vdf.binary_loads(f.read())

language = 'english'

STAT_TYPE_INT = '1'
STAT_TYPE_FLOAT = '2'
STAT_TYPE_AVGRATE = '3'
STAT_TYPE_BITS = '4'

achievements_out = []
stats_out = []

for appid in schema:
    sch = schema[appid]
    stat_info = sch['stats']
    for s in stat_info:
        stat = stat_info[s]
        if stat['type'] == STAT_TYPE_BITS:
            achs = stat['bits']
            for ach_num in achs:
                out = {}
                ach = achs[ach_num]
                out["hidden"] = '0'
                for x in ach['display']:
                    value = ach['display'][x]
                    if x == 'name':
                        x = 'displayName'
                    if x == 'desc':
                        x = 'description'
                    if x == 'Hidden':
                        x = 'hidden'
                    if type(value) is dict:
                        if language in value:
                            value = value[language]
                        else:
                            value = ''
                    out[x] = value
                out['name'] = ach['name']
                achievements_out += [out]
        else:
            out = {}
            out['default'] = 0
            out['name'] = stat['name']
            if stat['type'] == STAT_TYPE_INT:
                out['type'] = 'int'
            elif stat['type'] == STAT_TYPE_FLOAT:
                out['type'] = 'float'
            elif stat['type'] == STAT_TYPE_AVGRATE:
                out['type'] = 'avgrate'
            if 'Default' in stat:
                out['default'] = stat['Default']

            stats_out += [out]
        # print(stat_info[s])



output_ach = json.dumps(achievements_out, indent=4)
output_stats = ""
for s in stats_out:
    output_stats += "{}={}={}\n".format(s['name'], s['type'], s['default'])

# print(output_ach)
# print(output_stats)

config_directory = os.path.join(sys.argv[1] + "_output", "steam_settings")
if not os.path.exists(config_directory):
    os.makedirs(config_directory)

with open(os.path.join(config_directory, "achievements.json"), 'w') as f:
    f.write(output_ach)

with open(os.path.join(config_directory, "stats.txt"), 'w') as f:
    f.write(output_stats)

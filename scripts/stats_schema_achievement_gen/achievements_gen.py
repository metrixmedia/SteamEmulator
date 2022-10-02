import vdf
import sys
import os
import json


STAT_TYPE_INT = '1'
STAT_TYPE_FLOAT = '2'
STAT_TYPE_AVGRATE = '3'
STAT_TYPE_BITS = '4'

def generate_stats_achievements(schema, config_directory):
    schema = vdf.binary_loads(schema)
    # print(schema)
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
                        out[x] = value
                    out['name'] = ach['name']
                    if 'progress' in ach:
                        out['progress'] = ach['progress']
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
                elif 'default' in stat:
                    out['default'] = stat['default']

                stats_out += [out]
            #print(stat_info[s])



    output_ach = json.dumps(achievements_out, indent=4)
    output_stats = ""
    for s in stats_out:
        default_num = 0
        if (s['type'] == 'int'):
            try:
                default_num = int(s['default'])
            except ValueError:
                default_num = int(float(s['default']))
        else:
            default_num = float(s['default'])
        output_stats += "{}={}={}\n".format(s['name'], s['type'], default_num)

    # print(output_ach)
    # print(output_stats)

    if not os.path.exists(config_directory):
        os.makedirs(config_directory)

    with open(os.path.join(config_directory, "achievements.json"), 'w') as f:
        f.write(output_ach)

    with open(os.path.join(config_directory, "stats.txt"), 'w', encoding='utf-8') as f:
        f.write(output_stats)

    return (achievements_out, stats_out)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("format: {} UserGameStatsSchema_480.bin".format(sys.argv[0]))
        exit(0)


    with open(sys.argv[1], 'rb') as f:
        schema = f.read()

    generate_stats_achievements(schema, os.path.join("{}".format( "{}_output".format(sys.argv[1])), "steam_settings"))

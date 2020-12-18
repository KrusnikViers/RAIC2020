#!/usr/bin/python3

import datetime
import itertools
import json
import subprocess
import time
from operator import itemgetter

results = [[0, 0],
           [0, 0],
           [0, 0],
           [0, 0]]
positions_of_clients = list(itertools.permutations([0, 1, 2, 3]))
script_start_time = time.time()

RUNS_COUNT = len(positions_of_clients) * 4
OUTPUT_FILE = "batch-results.txt"


for i in range(0, RUNS_COUNT):
    test_start_time = time.time()
    print("Launching test #{} of {}".format(i + 1, RUNS_COUNT))
    pos_to_client = positions_of_clients[i % len(positions_of_clients)]
    client_to_pos = {value: index for index, value in enumerate(pos_to_client)}

    runner = subprocess.Popen(["aicup_runner.exe",
                               "--config", "all_tcp_config.json",
                               "--batch-mode",
                               "--save-results", OUTPUT_FILE])
    time.sleep(0.5)
    subprocess.Popen(["..\\out\\Release\\aicup2020.exe", "localhost",
                      "3100{}".format(client_to_pos[0] + 1)],
                     stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    subprocess.Popen(["..\\out\\Release\\aicup2020.exe", "localhost",
                      "3100{}".format(client_to_pos[1] + 1)],
                     stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    subprocess.Popen(["latest_sent.exe", "localhost",
                      "3100{}".format(client_to_pos[2] + 1)],
                     stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    subprocess.Popen(["latest_sent.exe", "localhost",
                      "3100{}".format(client_to_pos[3] + 1)],
                     stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    runner.communicate()

    with open(OUTPUT_FILE) as output:
        output_results = json.load(output)['results']
        local_results = [ {
                            'client': j,
                            'score': output_results[client_to_pos[j]]
                           } for j in range(0, 4) ]

        local_results = sorted(local_results, key=itemgetter('score'), reverse=True)
        for j in range(0, 4):
            results[local_results[j]['client']][0] += j + 1
            results[local_results[j]['client']][1] += local_results[j]['score']

    print('Time elapsed: {}'.format(datetime.timedelta(seconds=(time.time() - test_start_time))))
    print('Total time elapsed: {}'.format(datetime.timedelta(seconds=(time.time() - script_start_time))))
    for index, result in enumerate(results):
        print("Client #{}: {} place with {}k points average".format(
            index + 1,
            round(float(result[0]) / (i + 1), 2),
            round(float(result[1]) / ((i + 1) * 1000)), 1))
    print("\n------------------\n")

# -*- coding: utf-8 -*-
#
# Copyright (c) 2018 Baidu, Inc. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
A simple console program to emulate a bot for user to interact with

"""

import json
import os
import random
import requests
import sys


def main(bot_id, token):
    """
    Bot emulator
    Read user input and get bot response

    """
    # Url of dmkit, change this if you are runnning dmkit in a different ip/port.
    url = "http://127.0.0.1:8010/search?access_token=" + token
    
    json_header = {'Content-Type': 'application/json'}
    user_id = str(random.randint(1, 100000))
    session = "{\"session_id\":\"1\"}"
    while True:
        print "-----------------------------------------------------------------"
        user_input = raw_input("input:\n")
        user_input = user_input.strip()
        if not user_input:
            continue
        logid = str(random.randint(100000,999999))
        payload = {
            "version": "2.0",
            "bot_id": bot_id,
            "log_id": logid,
            "request": {
                "user_id": user_id,
                "query": user_input,
                "query_info": {
                    "type": "TEXT",
                    "source": "ASR",
                    "asr_candidates": []
                },
                "updates": "",
                "client_session": "{\"client_results\":\"\", \"candidate_options\":[]}",
                "bernard_level": 0
            },
            "bot_session": session
        }
        obj = requests.post(url, data=json.dumps(payload), headers=json_header).json()
        print "BOT:"
        # Not a valid response.
        if obj['error_code'] != 0:
            print "ERROR: %s" % obj['error_msg']
            continue
        
        session = obj['result']['bot_session']
        response = obj['result']['response']
        action = response['action_list'][0]
        # For clarify or failed responses which dmkit does not take actions.
        if action['type'] != 'event':
            print action['say']
            continue
        
        custom_reply = json.loads(action['custom_reply'])
        if custom_reply['event_name'] != 'DM_RESULT':
            print 'ERROR:unknown event name %s' % custom_reply['event_name']
            continue
        # This is a dmkit response
        dm_result = json.loads(custom_reply['result'])
        for item in dm_result['result']:
            if item['type'] == 'tts':
                print item['value']


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Usage: %s %s %s" % (sys.argv[0], "[bot_id]", "[access_token]")
    else:
        main(sys.argv[1], sys.argv[2])

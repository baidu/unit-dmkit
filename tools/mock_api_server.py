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
A mock server to provide api which DMKit can access to retrieve resources

"""

from flask import Flask
from flask import request
app = Flask(__name__)

@app.route("/hotel/search")
def hotel_search():
    location = request.args.get('location')
    return "如家酒店、四季酒店和香格里拉酒店"

@app.route("/hotel/book")
def hotel_book():
    time = request.args.get('time')
    hotel = request.args.get('hotel')
    room_type = request.args.get('room_type')
    return "OK"


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
Parser for .xml file 

"""
import xml.etree.ElementTree as ET
import re 
import codecs
import json
import sys


class Node(object):
    """
    this class is used to denote the node in .xml file, which will be used in XmlParser
    """
    def __init__(self, node_id, node_type, node_status, node_nlg, state, params):
        self.node_id = node_id
        self.node_type = node_type
        self.node_status = node_status # intent for user node, empty for server node 
        self.node_nlg = node_nlg # slots for customer node, nlg for server node
        self.state = state
        self.from_nodes = [] # (node_type, node_id, arrow_value)
        self.to_nodes = [] # (node_type, node_id, arrow_value)
        self.params = params  # parameters for server node


class Arrow(object):
    """
    this class is used to denote the arrow in .xml file, which will be used in XmlParser
    """
    def __init__(self, arrow_source, arrow_target):
        self.arrow_source = arrow_source # cell id 
        self.arrow_target = arrow_target # cell id 
        self.arrow_text = ""


class XmlParser(object):
    """
    this class is used to represent the graph in .xml file as a class object, 
    all necessary information about the graph will be extracted from the .xml file.
    """
    def __init__(self, xml_data):
        # initialize the elementtree object
        self.root = ET.fromstring("".join(xml_data))

        # collections of nodes, arrows, policies and nlg responses. 
        self.__nodes = {}
        self.__arrows = {}
        self.__nlgs = []
        self.__user_nodes = []
        self.__policies = []  # the output json

        # methods to parse the element tree
        self.__parse_cells() # parse every cell to extract info of node and arrow 
        self.__connect_nodes() # connect nodes with the info from arrow 
        self.__extract_policy()

    def __clean_noise(self, value, stri='', noise=None):
        if not noise:
            noise = ['&nbsp;', '&nbsp', 'nbsp', '<br>', '<div>', '</div>', '<span>', '</span>']
        for n in noise:
            value = value.replace(n, stri)
        if not stri:
            value = value.replace('&amp;', '&')
        return value.strip()

    def __clean_span(self, value, stri=''):
        return self.__clean_noise(value, '', ['<span>', '</span>'])

    def __parse_cells(self):
        todo_cells = []
        state = 1
        for cell in self.root[0]:
            style = cell.get('style')
            cell_id = cell.get('id')
            # bot node 
            if style and style.startswith('rounded=1'):
                value = cell.get('value') + '<'
                value = self.__clean_span(value)
                re_nlg = re.compile(r'BOT.*?<')
                re_params = re.compile(r'PARAM.*?<')
                nlgs = []
                if re_nlg.findall(value): 
                    for n in re_nlg.findall(value):
                        n = self.__clean_noise(n[4:-1])
                        nlgs.append(n)
                params = []
                if re_params.findall(value):
                    for p in re_params.findall(value):
                        p = self.__clean_noise(p[6:-1])
                        params.append(p)    
                if 'BOT' not in value:
                    raise Exception('wrong shape is used in cell with text: ', nlg)                    
                if value:
                    node = Node(cell_id, 'server', '', nlgs, (3 - len(str(state))) * '0'\
                            + str(state), params)
                    self.__nodes[cell_id] = node 
                    state += 1
                continue
            # customer node
            if style and style.startswith('ellipse'):
                value = cell.get('value') + '<'
                value = self.__clean_span(value)
                re_status = re.compile(r'INTENT.*?<')
                status = re_status.findall(value)[0] 
                status = self.__clean_noise(status[status.find(':') + 1:-1])
                re_nlg = re.compile(r'SLOT.*?<')
                if re_nlg.findall(value):
                    nlg = self.__clean_noise(re_nlg.findall(value)[0][5:-1])
                else:
                    nlg = ""
                if 'INTENT' not in value:
                    raise Exception('wrong shape is used in cell with text: ', nlg)  
                if value:
                    node = Node(cell_id, 'customer', status, nlg, '', [])
                    self.__nodes[cell_id] = node
                    self.__user_nodes.append(node)
                continue 
            # arrow node
            if cell.get('edge'):
                source = cell.get('source')
                target = cell.get('target')
                text = cell.get('value')
                if not target:
                    raise Exception('some arrow is not pointing to anything')  
                else:
                    arrow = Arrow(source, target)
                    if text:
                        arrow.arrow_text = self.__clean_noise(text)
                    self.__arrows[cell_id] = arrow
                continue
            # judge node
            if style and style.startswith('rhombus'):
                value = cell.get('value') + '<'
                value = self.__clean_span(value)
                re_params = re.compile(r'PARAM.*?<')
                params = []
                if re_params.findall(value):
                    for p in re_params.findall(value):
                        p = self.__clean_noise(p[6:-1])
                        params.append(p)
                if 'PARAM' not in value:
                    raise Exception('wrong shape is used in cell with text: ', value)                    
                if value:
                    node = Node(cell_id, 'judge', '', '', '', params)
                    self.__nodes[cell_id] = node 
                continue
            todo_cells.append(cell)
        # false initial
        self.__false_initial = Node(-1, "", "", "", "", "")
        self.__nodes[-1] = self.__false_initial 

        for cell in todo_cells:
            style = cell.get('style')
            parent_id = cell.get('parent')
            value = cell.get('value')
            if style and style.startswith('text'):
                if parent_id in self.__arrows:
                    self.__arrows[parent_id].arrow_text = self.__clean_noise(value)
                else:
                    raise Exception("there is a bad cell with text", value)

    def __connect_nodes(self):
        for (arrow_id, arrow) in self.__arrows.items():
            if not arrow.arrow_source:
                source_node = self.__false_initial
            else:
                source_node = self.__nodes[arrow.arrow_source]
            target_node = self.__nodes[arrow.arrow_target]
            # update source node and target node 
            source_node.to_nodes.append((target_node.node_type, target_node.node_id, 
                        arrow.arrow_text))
            self.__nodes[arrow.arrow_source] = source_node 
            target_node.from_nodes.append((source_node.node_type, 
                                           source_node.node_id, arrow.arrow_text))
            self.__nodes[arrow.arrow_target] = target_node

    def __extract_policy(self):
        for node in self.__user_nodes:
            intent = node.node_status
            slots = node.node_nlg.replace(' ', '').split(',')
            if slots[0] == "":
                slots = []
            params = []
            output = []
            if len(node.to_nodes) > 0:
                dir_to_node_info = node.to_nodes[0]
                dir_to_node = self.__nodes[dir_to_node_info[1]]
                # points to one branch
                if dir_to_node.node_type == "server":
                    for p in dir_to_node.params:                        
                        p = p.replace(" ", "")
                        c1 = p.find(":")
                        c2 = p.find("=")
                        pname = p[c1 + 1:c2]                      
                        ptype = p[0:c1]                        
                        pvalue = p[c2 + 1:]                        
                        param = {"name":pname, "type":ptype, "value":pvalue}
                        params.append(param)
                    next = {}
                    next["assertion"] = []
                    next["session"] = {"state": dir_to_node.state, "context": {}}
                    results = []
                    for n in dir_to_node.node_nlg:
                        result = {}
                        result["type"] = "tts"
                        alt = n.replace(" ", "").split('|')
                        if len(alt) == 1:
                            result["value"] = alt[0]
                        else:
                            result["value"] = alt
                        results.append(result)
                    next["result"] = results
                    output.append(next)
                # points to multiple branch
                elif dir_to_node.node_type == "judge":
                    # add params in judge nodes
                    for p in dir_to_node.params:                        
                        p = p.replace(" ", "")
                        c1 = p.find(":")
                        c2 = p.find("=")
                        pname = p[c1 + 1:c2]                      
                        ptype = p[0:c1]                        
                        pvalue = p[c2 + 1:]
                        param = {"name":pname, "type":ptype, "value":pvalue}
                        params.append(param)
                    # extract policies from bot nodes pointed to by the judge node
                    for to_node_info in dir_to_node.to_nodes:
                        to_node = self.__nodes[to_node_info[1]]
                        nexts = []
                        condition = to_node_info[2].replace(" ", "")
                        # A condition can contain either '&&'s or '||'s, but not both
                        if '&&' in condition:
                            conditions = condition.split('&&')
                            assertions = []
                            for cond in conditions:
                                cut = cond.find(":")
                                assertion = {}
                                assertion["type"] = cond[0:cut]
                                assertion["value"] = cond[cut + 1:]
                                assertions.append(assertion)
                            next = {}
                            next["assertion"] = assertions
                            next["session"] = {"state": to_node.state, "context": {}}
                            nexts.append(next)
                        elif '||' in condition:
                            conditions = condition.split('||')
                            for cond in conditions:
                                next = {}
                                assertion = {}
                                cut = cond.find(":")
                                assertion["type"] = cond[0:cut]
                                assertion["value"] = cond[cut + 1:]
                                next["assertion"] = [assertion]
                                next["session"] = {"state": to_node.state, "context": {}}
                                nexts.append(next)
                        else:
                            assertion = {}
                            cut = condition.find(":")
                            assertion["type"] = condition[0:cut]
                            assertion["value"] = condition[cut + 1:]
                            next = {}
                            next["assertion"] = [assertion]
                            next["session"] = {"state": to_node.state, "context": {}}
                            nexts.append(next)

                        results = []
                        for n in to_node.node_nlg:
                            result = {}
                            result["type"] = "tts"
                            alt = n.replace(" ", "").split('|')
                            if len(alt) == 1:
                                result["value"] = alt[0]
                            else:
                                result["value"] = alt
                            results.append(result)
                        for nxt in nexts:
                            nxt["result"] = results
                            output.append(nxt)
                        for p in to_node.params:                        
                            p = p.replace(" ", "")
                            c1 = p.find(":")
                            c2 = p.find("=")
                            pname = p[c1 + 1:c2]                      
                            ptype = p[0:c1]                        
                            pvalue = p[c2 + 1:]                        
                            param = {"name":pname, "type":ptype, "value":pvalue}
                            # merge
                            if param not in params:
                                params.append(param)

            for from_node_info in node.from_nodes:
                if from_node_info:
                    policy = {}
                    from_node = self.__nodes[from_node_info[1]]
                    trigger = {}
                    trigger["intent"] = intent
                    trigger["slots"] = slots 
                    trigger["state"] = from_node.state
                    policy["trigger"] = trigger
                    policy["params"] = params
                    policy["output"] = output
                    self.__policies.append(policy)

    def write_json(self):
        """
        this method is used to return the parsed json for the .xml input
        """
        return [json.dumps(self.__policies, ensure_ascii=False,\
                indent=2, sort_keys=True).encode('utf8')]


def run(data):
    """
    runs the parser and returns the parsed json
    """
    ps = XmlParser(data)
    return ps.write_json()

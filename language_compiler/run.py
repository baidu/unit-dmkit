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
Compiler Executor.

"""

import codecs
import ConfigParser
import importlib
import os
import sys


def main():
    """
    Compiler Executor
    process files in data_path and generate system json configuration file

    """
    current_dir = os.path.dirname(os.path.abspath(__file__))
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))

    config = ConfigParser.ConfigParser()
    config.read(current_dir + "/settings.cfg")
    
    compile_types = config.get('compiler', 'compile_types')
    compile_types = compile_types.replace(' ', '').split(',')

    data_path = current_dir + '/' + config.get('data', 'data_path')
    for dirpath, dirnames, filenames in os.walk(data_path):
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            fname, extension = os.path.splitext(filename)
            extension = extension[1:]
            if extension not in compile_types:
                continue
            input_lines = []
            with open(filepath, 'r') as f:
                input_lines = f.readlines()
            compiler_module = importlib.import_module('compiler_' + extension)
            compiler_function = getattr(compiler_module, 'run')
            output_lines = compiler_function(input_lines)
            with open(os.path.join(dirpath, fname + ".json"), 'w') as f:
                for line in output_lines:
                    f.write("%s\n" % line)

if __name__ == "__main__":
    main()

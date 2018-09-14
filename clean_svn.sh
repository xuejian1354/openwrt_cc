#!/bin/bash

#Help to clean the .svn in the files

find . -name .svn|xargs rm -rf

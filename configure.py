#!/usr/bin/python
import os
import sys
import subprocess

CXX      = 'g++'
AR       = 'ar'
MAKE     = 'make'
CXXFLAGS = '-std=c++17 -O0 -ggdb3 -Wall -Werror -I../gtest'

TLD = os.path.dirname(sys.argv[0])+'/'
PWD = os.getcwd()+'/'

class Build:
    def __init__(self):
        self.input_files = []
        self.output_file = ''
        self.src_dir = ''
        self.dependencies = []
        self.target_type = 0
        self.cxxflags = ''
        self.linkflags = ''
    def add_include_paths(self, p):
        self.cxxflags = self.cxxflags + ' ' +' '.join(['-I'+TLD+i for i in p])
    def set_cxxflags(self, f):
        self.cxxflags = f
    def set_linkflags(self, f):
        self.linkflags = f
    def set_src_dir(self, d):
        self.src_dir = d
    def add_src_files(self, f):
        self.name = f
        self.input_files.extend(f)
    def target_executable(self, f):
        self.name = f +"_build"
        self.output_file = f
        self.target_type = 0
    def target_archive(self, f):
        self.name = f +"_build"
        self.output_file = f
        self.target_type = 1
    def add_dependencies(self, d):
        self.dependencies.extend(d)
    def generate_make(self):
        output = ''
        objects = [self.name+'/'+i+'.o' for i in self.input_files]
        deps    = [self.name+'/'+i+'.d' for i in self.input_files]
        srcs    = [self.src_dir+i for i in self.input_files]
        print objects
        print deps
        print srcs
        output = output + '-include '+' '.join(deps) + '\n'
        output = output + self.output_file + ':' + ' '.join(self.dependencies)+' '+' '.join(objects) + '\n'
        # target rule
        if (self.target_type == 0):
            output = output + '\t'+ CXX + ' ' + ' '.join(objects) + ' ' + ' '.join(self.dependencies) + ' ' + self.linkflags +  ' -o ' + self.output_file + '\n'
        else:
            output = output + '\t'+ AR + ' rcs ' + self.linkflags + ' ' + self.output_file + ' ' + ' '.join(objects) + '\n'

        # object rule
        for i in range(len(self.input_files)):
            output = output + objects[i] + ':' + srcs[i] + '\n'
            output = output +'\t@mkdir -p ' + os.path.dirname(objects[i]) + '\n'
            output = output +'\t@echo Building '+objects[i]+'..\n'
            output = output +'\t@'+ CXX + ' -MMD ' + self.cxxflags + ' -c ' + srcs[i] + ' -o ' + objects[i] + '\n'

        return output;

def clean_filenames(a):
    return [i.strip().replace('./','') for i in a]

print 'configuring for testing'

print 'TLD is ' + TLD
print 'PWD is ' + PWD

LOGLESS_SOURCES  = []
SPAWNER_SOURCES  = []

print "LOGLESS_SOURCES", LOGLESS_SOURCES
print "SPAWNER_SOURCES", SPAWNER_SOURCES

logless = Build()
logless.set_cxxflags(CXXFLAGS)
logless.add_include_paths(['src/'])
logless.set_src_dir(TLD+'src/')
logless.add_src_files(["Logger.cpp"])
logless.target_archive('logless.a')

spawner = Build()
spawner.set_cxxflags(CXXFLAGS)
spawner.add_include_paths(['src/'])
spawner.set_src_dir(TLD+'src/')
spawner.add_src_files(["spawner.cpp"])
spawner.add_dependencies(['logless.a'])
spawner.set_linkflags("-lpthread")
spawner.target_executable('spawner')

test = Build()
test.set_cxxflags(CXXFLAGS)
test.add_include_paths(['src/'])
test.set_src_dir(TLD+'test/')
test.add_src_files(["main.cpp"])
test.add_dependencies(['logless.a'])
test.set_linkflags("-lpthread")
test.target_executable('test')

with open('Makefile','w+') as mf:
    mf.write(logless.generate_make())
    mf.write(spawner.generate_make())
    mf.write(test.generate_make())
    mf.write("test_run : spawner test\n")
    mf.write("\tobjcopy -O binary --only-section=.rodata test test.rodata\n")
    mf.write("\t./test\n")
    mf.write("\t./spawner test.rodata log.bin exiteof > test.log\n")
    mf.write("\techo log is written at test.log\n")


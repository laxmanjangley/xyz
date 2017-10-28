#!/bin/bash
import os
from glob import glob
from os import path
import shlex, time, logging, os, sys
from subprocess import Popen, PIPE, STDOUT
from threading import Thread
from time import sleep

def _mkdir(newdir):
    if os.path.isdir(newdir):
        pass
    elif os.path.isfile(newdir):
        raise OSError("a file with the same name as the desired " \
                      "dir, '%s', already exists." % newdir)
    else:
        head, tail = os.path.split(newdir)
        if head and not os.path.isdir(head):
            _mkdir(head)
        if tail:
            os.mkdir(newdir)

logger = logging.getLogger()
logger.setLevel(logging.INFO)
hdlr = logging.FileHandler('/var/tmp/myapp.log')
logger.addHandler(hdlr) 

def threaded_function():
	package("python -m SimpleHTTPServer 15255 ")	

def log_subprocess_output(pipe):
    for line in iter(pipe.readline, b''):
        logger.info(line)


def package(ext):
	command = ext
	print command
	logging.info(command)
	args = shlex.split(command)
	logging.info(args)
	pack = time.time()
	process = Popen(args, stdout=PIPE, stderr=STDOUT)
	#output =
	with process.stdout:
	    log_subprocess_output(process.stdout)
	#exitcode = process.wait()
	return process.wait()
	#return output

sub_dirs=glob('*/')
cur_dir=os.getcwd()
for dirs in sub_dirs:
	path_cre=cur_dir+"/"+dirs
	torrent_file=cur_dir+"/.."+"/tmp/"+dirs[:len(dirs)-1]+".torrent"
	print torrent_file
	package("./uri "+path_cre +" -o "+torrent_file)
	#os.system("./uri "+path +" -o "+torrent_file)

_mkdir("/home/next/Desktop/tmp")
os.chdir("/home/next/Desktop/tmp")
print os.system("pwd")

thread = Thread(target = threaded_function)
thread.start()
print "thread finished...exiting"


cur_dir_sup= os.getcwd();
files = filter(path.isfile, os.listdir(cur_dir_sup))

os.chdir(cur_dir)

final_magnet=""
for i in files:
	print i
	final_magnet=final_magnet+" "+cur_dir_sup+"/"+i+" "+cur_dir
print final_magnet

package("./a.out "+final_magnet)


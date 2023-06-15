#!/usr/bin/env python

import json
import sys
from TracerUtils import *


def getChoice():
    menu = "Menu:\n"
    menu += " p: print trace\n"
    menu += " s: print stats\n"
    menu += " x: explore\n"
    menu += " q: quit\n"
    choice = input(menu)
    return choice


def getDepthLimit():
    menu = "Max depth?: "
    n = int(input(menu))
    return n


def getTargetDepth():
    menu = "Target depth?: "
    n = int(input(menu))
    return n


def getMinOrMax():
    menu = "0: min\n"
    menu += "1: max\n"
    n = int(input(menu))
    return n


def getEventName():
    menu = "Enter event name\n"
    event_name = input(menu)
    return event_name


def selectProcess(process_ids):
    menu = "Select process:"
    for pid in process_ids:
        menu += " " + str(pid)
    menu += "\n"
    n = int(input(menu))
    return n


def selectThread(selected_process):
    i = 0
    menu = "Select thread:\n"
    for pid in thread_ids:
        menu += " " + str(i) + ": " + str(pid) + "\n"
        i += 1
    n = int(input(menu))
    return thread_ids[n]


if __name__ == "__main__":
    event_list = []
    for i in range(1, len(sys.argv)):
        with open(sys.argv[i]) as f:
            event_list += extractEventList(json.load(f))
    process_ids = getProcessIds(event_list)
    selected_process = process_ids[0]
    if len(process_ids) > 1:
        selected_process = selectProcess(process_ids)
    thread_ids = getThreadIds(event_list, selected_process)
    selected_thread = thread_ids[0]
    if len(thread_ids) > 1:
        selected_thread = selectThread(thread_ids)
    while 1:
        c = getChoice()
        s = ""
        if "p" == c:
            n = getDepthLimit()
            s = generateTraceString(event_list, n, selected_process, selected_thread)
        elif "s" == c:
            n = getTargetDepth()
            s = generateTotalsString(event_list, n, selected_process, selected_thread)
        elif "x" == c:
            c = getMinOrMax()
            if 0 == c:
                event_name = getEventName()
                proc_id, duration = getIdOfMinProc(event_list, event_name)
                s = event_name + ": min proc = " + str(proc_id) + " (" + str(duration) + ")"
        else:
            break
        print(s)

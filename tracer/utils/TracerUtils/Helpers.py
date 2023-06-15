from .Event import Event


def extractEventList(parsed_json_events):
    event_list = []
    for e in parsed_json_events:
        event_list.append(Event(e))
    return event_list


def convertFromMicroseconds(duration):
    seconds = float(duration) / 1000000
    return seconds


def getIdOfMinProc(event_list, event_name):
    min_rank = 0
    min_duration = 9.9e99
    start = 0
    for e in event_list:
        if event_name == e.name:
            if "B" == e.ph:
                start = e.time_stamp
            elif "E" == e.ph:
                duration = e.time_stamp - start
                if duration < min_duration:
                    min_duration = duration
                    min_rank = e.process_id
    return min_rank, convertFromMicroseconds(min_duration)


def generateTotalsString(event_list, target_depth, process_id, thread_id):
    depth = 0
    event_totals = {}
    unfinished_events = {}
    for e in event_list:
        name = e.name
        timestamp = e.time_stamp
        if "B" == e.ph:
            if name not in unfinished_events:
                unfinished_events[name] = timestamp
                depth += 1
        elif "E" == e.ph:
            if name in unfinished_events:
                depth -= 1
                duration = timestamp - unfinished_events[name]
                elapsed_seconds = convertFromMicroseconds(duration)
                del unfinished_events[name]
                if depth != target_depth:
                    continue
                if name in event_totals:
                    event_totals[name] += elapsed_seconds
                else:
                    event_totals[name] = elapsed_seconds
    if not event_totals:
        return "no events at target depth: %d" % target_depth
    longest_event_width = len(max(event_totals, key=len))
    totals_string = ""
    for event_name, event_duration in sorted(event_totals.items(), key=lambda item: item[1], reverse=True):
        totals_string += "%s %e seconds\n" % (event_name.rjust(longest_event_width), event_duration)
    return totals_string


def generateTraceString(event_list, depth_limit, selected_process, selected_thread):
    trace_string = ""
    depth = 0
    unfinished_events = {}
    for e in event_list:
        if e.process_id != selected_process or e.thread_id != selected_thread:
            continue
        name = e.name
        if "B" == e.ph:
            if name not in unfinished_events:
                if depth <= depth_limit:
                    trace_string += "%s%s\n" % (''.rjust(2 * depth), name.rjust(2 * depth))
                unfinished_events[name] = e.time_stamp
                depth += 1
        elif "E" == e.ph:
            if name in unfinished_events:
                depth -= 1
                duration = e.time_stamp - unfinished_events[name]
                elapsed_seconds = convertFromMicroseconds(duration)
                del unfinished_events[name]
                if depth <= depth_limit:
                    trace_string += "%s%s (%s seconds)\n" % (''.rjust(2 * depth), name, elapsed_seconds)
    return trace_string


def getProcessIds(event_list):
    ids = set()
    for e in event_list:
        ids.add(e.process_id)
    id_list = []
    for id in ids:
        id_list.append(id)
    return id_list


def getThreadIds(event_list, selectedProcess):
    ids = set()
    for e in event_list:
        if e.process_id == selectedProcess:
            ids.add(e.thread_id)
    id_list = []
    for id in ids:
        id_list.append(id)
    return id_list



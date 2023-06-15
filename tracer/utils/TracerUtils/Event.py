class Event:
    name = ""
    process_id = 0
    thread_id = 0
    time_stamp = 0
    ph = ""

    def __init__(self, json_event):
        self.name = json_event["name"]
        self.process_id = int(json_event["pid"])
        self.thread_id = int(json_event["tid"])
        self.time_stamp = int(json_event["ts"])
        self.ph = json_event["ph"]

import unittest
import json
from TracerUtils import convertFromMicroseconds, Event, extractEventList, getProcessIds, getThreadIds, generateTraceString, generateTotalsString


class TraceTestCase(unittest.TestCase):
  def setUp(self):
    s = '[{"cat": "xx", "pid": 7, "tid": 576, "ts": "172045", "ph": "B", "name": "First event"},'
    s += '{"cat": "xx", "pid": 7, "tid": 576, "ts": "172060", "ph": "B", "name": "First event"},'
    s += '{"cat": "category", "pid": 7, "tid": 576, "ts": "83", "ph": "C", "name": "Memory (MB)", "args": {"Memory (MB)": 97}},'
    s += '{"cat": "xx", "pid": 7, "tid": 576, "ts": "552045", "ph": "E", "name": "First event"},'
    s += '{"cat": "xx", "pid": 7, "tid": 576, "ts": "552050", "ph": "E", "name": "First event"}]'

    self.parsed_json_events = json.loads(s)

  def testMicroSecondConversion(self):
    microseconds = 1234000
    seconds = convertFromMicroseconds(microseconds)
    assert 1.234 == seconds

  def testConstructEvent(self):
    first_event = self.parsed_json_events[0]
    e = Event(first_event)
    assert 7 == e.process_id

  def testExtractEventList(self):
    event_list = extractEventList(self.parsed_json_events)
    assert 5 == len(event_list)
    assert "First event" == event_list[0].name
    process_ids = getProcessIds(event_list)
    assert 1 == len(process_ids)
    assert 7 == process_ids[0]
    thread_ids = getThreadIds(event_list,7)
    assert 1 == len(thread_ids)
    assert 576 == thread_ids[0]

  def testGetTraceString(self):
    event_list = extractEventList(self.parsed_json_events)
    max_depth = 0
    process_id = 7
    thread_id = 576
    trace_string = generateTraceString(event_list,max_depth,process_id,thread_id)
    assert 2 == trace_string.count('\n')

  def testGetTotalsString(self):
    event_list = extractEventList(self.parsed_json_events)
    target_depth = 0
    process_id = 7
    thread_id = 576
    totals_string = generateTotalsString(event_list,target_depth,process_id,thread_id)
    assert 1 == totals_string.count('\n')



if __name__ == "__main__":
  unittest.main()

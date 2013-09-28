import unittest
from scanner import StringScanner
from scanner import StringRegexp


class TestScanner(unittest.TestCase):

    def setUp(self):
        self.string = 'hello'
        self.strscan = StringScanner(self.string)

    def tearDown(self):
        pass

    def test_eos(self):
        assert self.strscan.eos == False
        self.strscan.terminate()
        assert self.strscan.eos == True

    def test_getch(self):
        pass

    def test_peek(self):
        s = self.strscan
        r = s.peek(4)
        assert r == 'hell'

    def test_rest(self):
        s = self.strscan
        s.scan(StringRegexp('hel'))
        assert self.string == s.string

    def test_matched(self):
        s = self.strscan
        s.scan(StringRegexp('hel'))
        assert 'hel' == s.matched

    def test_pre_match(self):
        pass

    def test_post_match(self):
        pass

    def test_unscan(self):
        s = self.strscan
        s.skip(StringRegexp('he'))
        assert 'llo' == s.rest
        s.unscan()
        assert 0 == s.pos
        assert self.string == s.string

    def test_is_beginning_of_line(self):
        s = self.strscan
        assert s.bol == True

    def test_terminate(self):
        pass

    def test_scan_full(self):
        # TODO: scan_full default args
        s = self.strscan
        assert None == s.scan_full(StringRegexp("l"))
        assert 'he' == s.scan_full(StringRegexp("he"), True, True)
        assert 2 == s.pos
        #assert 'll' == s.scan_full(StringRegexp('ll'), advance_pointer=False)
        #assert 2 == s.pos
        #assert 2 == s.scan_full(StringRegexp('ll'), return_string=False, advance_pointer=False)
        #assert 2 == s.pos

if __name__ == '__main__':
    unittest.main()

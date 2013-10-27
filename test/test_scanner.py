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
        s = self.strscan
        pos = s.pos
        assert 'h' == s.getch()
        assert s.pos == (pos + 1)
        s.getch()
        assert 'l' == s.getch()

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
        s = self.strscan
        assert 2 == s.skip(StringRegexp('he'))
        assert 'll' == s.scan(StringRegexp('ll'))
        assert 'he' == s.pre_match()

    def test_post_match(self):
        s = self.strscan
        s.skip(StringRegexp('he'))
        s.scan(StringRegexp('ll'))
        assert 'o' == s.post_match()

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
        s = self.strscan
        s.getch()
        s.terminate()
        assert s.pos == len(self.string)
        #assert s.match == None  # what is "match"?

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

    #def test_search_full(self):
    #    s = self.strscan
    #    assert 'he' == s.search_full('e')
    #    assert 2 == s.pos
    #    assert 'llo' == s.search_full('lo', advance_pointer=False)
    #    assert 2 == s.pos
    #    assert 3 == s.search_full('o', return_string=False, advance_pointer=False)

    def test_scan(self):
        s = self.strscan
        assert 0 == s.pos
        s.scan(StringRegexp('world'))
        s.scan(StringRegexp('luo'))
        assert 0 == s.pos
        assert 'hel' == s.scan(StringRegexp('hel'))
        assert 3 == s.pos

    def test_scan_until(self):
        s = self.strscan
        assert 'hel' == s.scan_until(StringRegexp('el'))
        assert 3 == s.pos

    # what is "upto"?
    #def test_scan_upto(self):
    #    s = self.strscan
    #    assert 'h' == s.scan('h')
    #    assert 'el' == s.scan_upto('lo')
    #    assert 3 == s.pos
    #    assert [0, 1, 3] == s.pos_history

    def test_skip(self):
        s = self.strscan
        assert 3 == s.skip(StringRegexp('hel'))

    def test_skip_unitl(self):
        s = self.strscan
        assert 3 == s.skip_until(StringRegexp('l'))

    def test_check(self):
        s = self.strscan
        assert 'hell' == s.check(StringRegexp('hell'))
        assert 0 == s.pos

    def test_check_until(self):
        s = self.strscan
        assert 'hell' == s.check(StringRegexp('hell'))
        assert 0 == s.pos

    def test_exist(self):
        s = self.strscan
        assert 3 == s.exist(StringRegexp('l'))
        assert 0 == s.pos

if __name__ == '__main__':
    unittest.main()

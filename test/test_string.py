# -*- coding: utf-8 -*-

import unittest
from scanner import StringScanner


class TestString(unittest.TestCase):

    def setUp(self):
        self.string = 'hello'
        self.strscan = StringScanner(self.string)

    def test_is_eos(self):
        assert self.strscan.is_eos == False
        self.strscan.terminate
        assert self.strscan.is_eos == True

    def test_getch(self):
        s = self.strscan
        pos = s.pos
        assert 'h' == s.getch
        assert s.pos == (pos + 1)
        s.getch
        assert 'l' == s.getch

    def test_peek(self):
        s = self.strscan
        r = s.peek(4)
        assert r == 'hell'

    def test_rest(self):
        s = self.strscan
        s.scan('hel')
        assert self.string == s.string

    def test_matched(self):
        s = self.strscan
        s.scan('hel')
        assert 'hel' == s.matched

    def test_pre_match(self):
        s = self.strscan
        assert 2 == s.skip('he')
        assert 'll' == s.scan('ll')
        assert 'he' == s.pre_match

    def test_post_match(self):
        s = self.strscan
        s.skip('he')
        s.scan('ll')
        assert 'o' == s.post_match

    def test_unscan(self):
        s = self.strscan
        s.skip('he')
        assert 'llo' == s.rest
        s.unscan
        assert 0 == s.pos
        assert self.string == s.string

    def test_is_beginning_of_line(self):
        s = self.strscan
        assert s.is_bol == True

    def test_terminate(self):
        s = self.strscan
        s.getch
        s.terminate
        assert s.pos == len(self.string)
        # FIXME match 丢了吗
        # match 就是原本的 self._match 呀，初始时为 None,
        # 后边是匹配到的结果呀呀呀
        # assert s.match == None  # what is "match"?

    def test_scan_full(self):
        s = self.strscan
        assert None == s.scan_full("l")
        assert 'he' == s.scan_full("he")
        assert 2 == s.pos
        assert 'll' == s.scan_full('ll', advance_pointer=False)
        assert 2 == s.pos
        assert 2 == s.scan_full('ll', advance_pointer=False, return_string=False)
        assert 2 == s.pos

    def test_search_full(self):
        s = self.strscan
        assert 'he' == s.search_full('e')
        assert 2 == s.pos
        assert 'llo' == s.search_full('lo', advance_pointer=False)
        assert 2 == s.pos
        assert 3 == s.search_full('o', advance_pointer=False, return_string=False)

    def test_scan(self):
        s = self.strscan
        assert 0 == s.pos
        s.scan('world')
        s.scan('luo')
        assert 0 == s.pos
        assert 'hel' == s.scan('hel')
        assert 3 == s.pos

    def test_scan_until(self):
        s = self.strscan
        assert 'hel' == s.scan_until('el')
        assert 3 == s.pos

    # what is "upto"?
    # FIXME 就是跳到匹配的位置，但是不匹配跳过的内容呀
    # See https://github.com/liluo/linguist/blob/master/linguist/libs/strscan.py
    # FIXME 上面那个 url 是我在火车上写的，不知道有没有写错地址
    #def test_scan_upto(self):
    #    s = self.strscan
    #    assert 'h' == s.scan('h')
    #    assert 'el' == s.scan_upto('lo')
    #    assert 3 == s.pos
    #    assert [0, 1, 3] == s.pos_history

    def test_skip(self):
        s = self.strscan
        assert 3 == s.skip('hel')

    def test_skip_unitl(self):
        s = self.strscan
        assert 3 == s.skip_until('l')

    def test_check(self):
        s = self.strscan
        assert 'hell' == s.check('hell')
        assert 0 == s.pos

    def test_check_until(self):
        s = self.strscan
        assert 'hell' == s.check('hell')
        assert 0 == s.pos

    def test_exist(self):
        s = self.strscan
        assert 3 == s.exists('l')
        assert 0 == s.pos

    # FIXME 我把你偷懒的 2 个 unittext 也拿过来的，检查一下 coords 嘛
    # def test_coords(self):
    #     s = StringScanner("abcdef\nghijkl\nmnopqr\nstuvwx\nyz")
    #     assert (0, 0, "abcdef") == s.coords
    #     s.pos += 4
    #     assert (0, 4, 'abcdef') == s.coords
    #     s.pos += 2
    #     assert (0, 6, 'abcdef') == s.coords
    #     s.pos += 1
    #     assert (1, 0, 'ghijkl') == s.coords
    #     s.pos += 8
    #     assert (2, 1, 'mnopqr') == s.coords

    # def test_text_coords(self):
    #     s = "abcdef\nghijkl\nmnopqr\nstuvwx\nyz"
    #     assert (0, 0, 'abcdef') == text_coords(s, 0)
    #     assert (0, 4, 'abcdef') == text_coords(s, 4)
    #     assert (0, 6, 'abcdef') == text_coords(s, 6)
    #     assert (1, 0, 'ghijkl') == text_coords(s, 7)
    #     assert (1, 4, 'ghijkl') == text_coords(s, 11)
    #     assert (2, 1, 'mnopqr') == text_coords(s, 15)

if __name__ == '__main__':
    unittest.main()

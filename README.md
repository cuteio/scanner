# Scanner
[![Build Status](https://travis-ci.org/cuteio/scanner.png?branch=master)](https://travis-ci.org/cuteio/scanner)
Scanner provides for lexical scanning operations on a String. Clone of [Ruby's StringScanner](http://ruby-doc.org/stdlib-2.0.0/libdoc/strscan/rdoc/StringScanner.html).

Here is an example of its usage:

```
from scanner import StringScanner, StringRegexp
s = StringScanner('This is an example string')
s.is_eos             # -> False

print s.scan(StringRegexp('\w+'))  # -> "This"
print s.scan(StringRegexp('\w+'))  # -> None
print s.scan(StringRegexp('\s+'))  # -> " "
print s.scan(StringRegexp('\s+'))  # -> None
print s.scan(StringRegexp('\w+'))  # -> "is"
s.is_eos                           # -> False

print s.scan(StringRegexp('\s+'))      # -> " "
print s.scan(StringRegexp('\w+'))      # -> "an"
print s.scan(StringRegexp('\s+'))      # -> " "
print s.scan(StringRegexp('\w+'))      # -> "example"
print s.scan(StringRegexp('\s+'))      # -> " "
print s.scan(StringRegexp('\w+'))      # -> "string"
s.is_eos                               # -> True

print s.scan(StringRegexp('\s+'))      # -> nil
print s.scan(StringRegexp('\w+'))      # -> nil
```

Scanning a string means remembering the position of a scan pointer, which is just an index. The point of scanning is to move forward a bit at a time, so matches are sought after the scan pointer; usually immediately after it.

Given the string "test string", here are the pertinent scan pointer positions:

```
  t e s t   s t r i n g
0 1 2 ...             1
                      0
```

When you scan for a pattern (a regular expression), the match must occur at the character after the scan pointer. If you use scan_until, then the match can occur anywhere after the scan pointer. In both cases, the scan pointer moves just beyond the last character of the match, ready to scan again from the next character onwards. This is demonstrated by the example above.


There are other methods besides the plain scanners. You can look ahead in the string without actually scanning. You can access the most recent match. You can modify the string being scanned, reset or terminate the scanner, find out or change the position of the scan pointer, skip ahead, and so on.

#### Advancing the Scan Pointer

```
getch
get_byte
scan
scan_until
skip
skip_until
```

#### Looking Ahead

```
check
check_until
exists
peek
```

#### Finding Where we Are

```
is_bol (beginning_of_line?)
is_eos
is_rest
rest_size
pos
```

#### Setting Where we Are

```
reset
terminate
pos=
```

####Match Data

```
matched
is_matched
matched_size
pre_match
post_match
```

#### Miscellaneous

```
concat
string
string=
unscan
```

### Changelog

__v0.1.0[2016-08-12]__
* Support Python 3.x

__v0.0.5[2014-02-07]__
* Fix the setter prototype and return value

__v0.0.4[2013-11-19]__
* Fix Python 2.7.3 `Segmentation fault`.

__v0.0.3[2013-11-17]__

__v0.0.2[2013-11-17]__
* It's worked.

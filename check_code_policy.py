#!/usr/bin/python
#
#  Check various C source code policy rules and issue warnings for offenders
#
#  Usage:
#
#    $ python check_code_policy.py src/*.c
#

import os
import sys
import re
import optparse

class Problem:
	filename = None
	linenumber = None
	line = None
	reason = None

	def __init__(self, filename, linenumber, line, reason):
		self.filename = filename
		self.linenumber = linenumber
		self.line = line
		self.reason = reason


re_trailing_ws = re.compile(r'^.*?\s$')
re_only_ws = re.compile(r'^\s*$')
re_leading_tab = re.compile(r'^[\t]+.*$')  # tabs are only used for indent
re_identifier = re.compile(r'[A-Za-z0-9_]+')

# These identifiers are wrapped in duk_config.h, and should only be used
# through the wrappers elsewhere.
rejected_plain_identifiers = {}

problems = []

re_repl_c_comments = re.compile(r'/\*.*?\*/', re.DOTALL)
re_repl_cpp_comments = re.compile(r'//.*?\n', re.DOTALL)
re_repl_string_literals_dquot = re.compile(r'''\"(?:\\\"|[^\"])*\"''')
re_repl_string_literals_squot = re.compile(r'''\'(?:\\\'|[^\'])*\'''')
re_repl_expect_strings = re.compile(r'/\*===.*?===*?\*/', re.DOTALL)
re_not_newline = re.compile(r'[^\n]+', re.DOTALL)

def removeCommentsAndLiterals(data):
	def repl_c(m):
		tmp = re.sub(re_not_newline, '', m.group(0))
		if tmp == '':
			tmp = ' '  # avoid /**/
		return '/*' + tmp + '*/'
	def repl_cpp(m):
		return '// removed\n'
	def repl_dquot(m):
		return '"' + ('.' * (len(m.group(0)) - 2)) + '"'
	def repl_squot(m):
		return "'" + ('.' * (len(m.group(0)) - 2)) + "'"

	data = re.sub(re_repl_c_comments, repl_c, data)
	data = re.sub(re_repl_cpp_comments, repl_cpp, data)
	data = re.sub(re_repl_string_literals_dquot, repl_dquot, data)
	data = re.sub(re_repl_string_literals_squot, repl_squot, data)
	return data

def removeExpectStrings(data):
	def repl(m):
		tmp = re.sub(re_not_newline, '', m.group(0))
		if tmp == '':
			tmp = ' '  # avoid /*======*/
		return '/*===' + tmp + '===*/'

	data = re.sub(re_repl_expect_strings, repl, data)
	return data

def checkTrailingWhitespace(lines, idx, filename):
	line = lines[idx]
	if len(line) > 0 and line[-1] == '\n':
		line = line[:-1]

	m = re_trailing_ws.match(line)
	if m is None:
		return

	raise Exception('trailing whitespace')

def checkCarriageReturns(lines, idx, filename):
	line = lines[idx]
	if not '\x0d' in line:
		return

	raise Exception('carriage return')

def subCheckMixedIndent(line, re):
	if re in line:
		# Mixed tab/space are only allowed after non-whitespace characters
		idx = line.index(re)
		tmp = line[0:idx]
		m = re_only_ws.match(tmp)
		if m is not None:
			raise Exception('mixed space/tab indent (idx %d)' % idx)

def checkMixedIndent(lines, idx, filename):
	subCheckMixedIndent(lines[idx], '\x20\x09')
	subCheckMixedIndent(lines[idx], '\x09\x20')


def checkLeadingTab(lines, idx, filename):
	line = lines[idx]
	m = re_leading_tab.match(line)
	if m is None:
		return
	raise Exception('leading tab (idx %d)' % idx)

def checkFixme(lines, idx, filename):
	line = lines[idx]
	if not 'FIXME' in line:
		return

	raise Exception('FIXME on line')


def processFile(filename, checkersRaw, checkersNoExpectStrings):
	f = open(filename, 'rb')
	dataRaw = f.read()
	f.close()

	dataNoComments = removeCommentsAndLiterals(dataRaw)   # no c/javascript comments, literals removed
	dataNoExpectStrings = removeExpectStrings(dataRaw)    # no testcase expect strings

	linesRaw = dataRaw.split('\n')
	linesNoComments = dataNoComments.split('\n')
	linesNoExpectStrings = dataNoExpectStrings.split('\n')

	def f(lines, checkers):
		for linenumber in xrange(len(lines)):
			for fun in checkers:
				try:
					fun(lines, linenumber, filename)  # linenumber is zero-based here
				except Exception as e:
					problems.append(Problem(filename, linenumber + 1, lines[linenumber], str(e)))

	f(linesRaw, checkersRaw)
	f(linesNoExpectStrings, checkersNoExpectStrings)

	# Last line should have a newline, and there should not be an empty line.
	# The 'split' result will have one empty string as its last item in the
	# expected case.  For a single line file there will be two split results
	# (the line itself, and an empty string).

	if len(linesRaw) == 0 or \
	   len(linesRaw) == 1 and linesRaw[-1] != '' or \
	   len(linesRaw) >= 2 and linesRaw[-1] != '' or \
	   len(linesRaw) >= 2 and linesRaw[-1] == '' and linesRaw[-2] == '':
		problems.append(Problem(filename, len(linesRaw), '(no line)', 'No newline on last line or empty line at end of file'))

	# First line should not be empty (unless it's the only line, len(linesRaw)==2)
	if len(linesRaw) > 2 and linesRaw[0] == '':
		problems.append(Problem(filename, 1, '(no line)', 'First line is empty'))

def asciiOnly(x):
	return re.sub(r'[\x80-\xff]', '#', x)

def main():
	parser = optparse.OptionParser()
	parser.add_option('--dump-vim-commands', dest='dump_vim_commands', default=False, help='Dump oneline vim command')
	(opts, args) = parser.parse_args()

	checkersRaw = []
	checkersRaw.append(checkCarriageReturns)
#	checkersRaw.append(checkFixme)

	checkersNoExpectStrings = []
	checkersNoExpectStrings.append(checkTrailingWhitespace)
	checkersNoExpectStrings.append(checkMixedIndent)
	checkersNoExpectStrings.append(checkLeadingTab)

	for filename in args:
		processFile(filename, checkersRaw, checkersNoExpectStrings)

	if len(problems) > 0:
		for i in problems:
			tmp = 'vim +' + str(i.linenumber)
			while len(tmp) < 10:
				tmp = tmp + ' '
			tmp += ' ' + str(i.filename) + ' : ' + str(i.reason)
			while len(tmp) < 80:
				tmp = tmp + ' '
			tmp += ' - ' + asciiOnly(i.line.strip())
			print(tmp)

		print '*** Total: %d problems' % len(problems)

		if opts.dump_vim_commands:
			cmds = []
			for i in problems:
				cmds.append('vim +' + str(i.linenumber) + ' "' + i.filename + '"')
			print ''
			print('; '.join(cmds))

		sys.exit(1)

	sys.exit(0)

if __name__ == '__main__':
	main()

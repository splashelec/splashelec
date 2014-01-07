#!/usr/bin/env python

import sys

class ProgressBar:
	""" produces a text based status display/progress bar
	
	To show the progess bar, call the object as a function.

	For example:
    
	>>> foo = ProgressBar(width=40)
	>>> foo(22)
	[=======>         22%                   ]
	
    On creation, its possible to give max. and min. values for the length.
    The value gives the complete length, including opening and closing brackets.
	"""
	def __init__(self, min = 0, max = 100, width = 80):
		self.progbar = ""
		self.min = min
		self.max = max
		self.width = width
		self.value = 0
		self.update(0)
	
	def update(self, value = 0):
		""" updates the progress bar
		
        If value is outside min. and maximum values, limit the displayed value.
		"""
		if value < self.min: value = self.min
		if value > self.max: value = self.max
		
		if value == self.value:
			return False
		self.value = value
		# calculate the amount finished
		percent_done = (float(value - self.min) / 
						float(self.max - self.min)) * 100.0
		percent_done = int(round(percent_done))
		
		max_char = self.width - 2
		num_hashes = int(round((percent_done / 100.0) * max_char))

		if num_hashes == 0:
			self.progbar = "[>%s]" % (' '*(max_char-1))
		elif num_hashes == max_char:
			self.progbar = "[%s]" % ('='*max_char)
		else:
			self.progbar = "[%s>%s]" % ('='*(num_hashes-1),
										' '*(max_char-num_hashes))

		# put the percent display about to the middle
		percent_position = (len(self.progbar) / 2) - len(str(percent_done)) 
		percent_str     = str(percent_done) + "%"
		self.progbar = ''.join([self.progbar[0:percent_position], percent_str,
								self.progbar[percent_position+len(percent_str):]])
		return True

	def __str__(self):
		""" returns the up-to-date progress bar 
		
        The length of the returned string corresponds to width given on creation
        (default is 80).
		"""
		return str(self.progbar)

	def __call__(self, value):
		""" updates the status display and transfers it to stream stdout,
            if it has changed.
		
		...writes first a  "carrige return" to overwrite the current line.
		"""
		if self.update(value):
			progbar = str(self)
			sys.stdout.write("\r" + progbar)
			sys.stdout.flush()

# a small test program for the progressbar class
#
# Puts a 60 character large progress bar to the console
# which grows slowly from 0 to 100 %.

if __name__ == '__main__':
	import time

	bar = ProgressBar(width=60)

	for x in range(0,101):
		time.sleep(0.10)
		bar(x)
	print ""


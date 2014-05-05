#!/usr/bin/env python

import gtk
import csv
import math
import re

PCBdataDir = 'exampleBoardData'
if PCBdataDir[len(PCBdataDir)-1] != '/':
   PCBdataDir = PCBdataDir + '/';

class Boardsize:
  def __init__(self):
    self.reader = csv.reader(open(PCBdataDir + 'board.size', 'rb'), delimiter=',', quotechar='"')
    for row in self.reader:
      # throw comments away
      if row[0][0] == "#": continue

      self.x = float(row[0].split()[0])
      self.xUnits = row[0].split()[1].lower()
      if self.xUnits == 'mil': self.x *= 25.4/1000
      elif self.xUnits == 'mm': pass
      else: raise Exception('units not recognized')

      self.y = float(row[1].split()[0])
      self.yUnits = row[1].split()[1].lower()
      if self.yUnits == 'mil': self.y *= 25.4/1000
      elif self.yUnits == 'mm': pass
      else: raise Exception('units not recognized')
      
  def get_size(self):
    return dict(x=self.x, y=self.y)

  def getX(self):
    return self.x

  def getY(self):
    return self.y


class Coordinates:
  def __init__(self, boardSize):
    unitsFound = False
    self.data = {}
    self.reader = csv.reader(open(PCBdataDir + 'board.xy', 'rb'), delimiter=',', quotechar='"')
    for row in self.reader:
      # throw comments away
      if row[0][0] == "#": # this is a comment line
        print row
        # match the line "# X,Y in mil.  rotation in degrees."
        p = re.compile('# X,Y in (?P<units>(mm|mil)).  rotation in degrees.')
        match = p.match(','.join(row))
        if match:
          units = match.group('units')
          if units == "mil": factor = 25.4/1000
          elif units == "mm": factor = 1
          else: raise Exception('Units not recognized')
          unitsFound = True
        continue
      refdes       = row[0]
      x            = float(row[3])*factor
      y            = float(row[4])*factor
      rotation     = float(row[5])
      mountingSide = row[6]

      # flip x coordinate if component has to be placed on bottom side
      if mountingSide == 'bottom': x = boardSize.getX() - x

      # create a dictionary that associates the refdes to the coordinates
      self.data[refdes] = dict(x=x, y=y, 
                               rotation=rotation, 
                               mountingSide=mountingSide)
    if not unitsFound : raise Exception('no units found in .xy file')


  def get_data(self):
    return self.data
    
class Bom:
  def __init__(self):
    self.reader = csv.reader(open(PCBdataDir + 'board.bom.csv', 'rb'), delimiter=',', quotechar='"')
    self.data = []
    
    
    for row in self.reader:
      try:
	lineNumber = int(row[0])
      except ValueError: pass # Throw away all non numbered lines.
      else:
	try:
	  quantity = int(row[1])
	except ValueError: quantity = 0
	footprint = row[2]
	value     = row[3]
	refdes    = row[4]
	stockId   = row[5]
        skip      = row[6]
        comment   = row[7]
	# There can be more than one refdes per line
	# recount!
	quantity = len(refdes.split())
	position = 0 # position of refdes in this line
        refdesList = refdes.split()
        if len(refdesList) == 0: 
          self.data.append(dict(
	    lineNumber = lineNumber,
	    position   = position,
	    quantity   = quantity,
	    footprint  = footprint,
	    value      = value,
	    refdes     = refdes,
	    stockId    = stockId,
            skip       = skip,
            comment    = comment))
                        
	for refdes in refdesList:
	  self.data.append(dict(
	    lineNumber = lineNumber,
	    position   = position,
	    quantity   = quantity,
	    footprint  = footprint,
	    value      = value,
	    refdes     = refdes,
	    stockId    = stockId,
            skip       = skip,
            comment    = comment))
	  position += 1  
 	
    self.lineIndex = 0
    self.refIndex  = 0


  def get_line(self):
    return self.data[self.lineIndex]

    
  def set_lineIndex(self, index):
    if index < 0: index = 0
    if index >= len(self.data): index = len(self.data) - 1
    self.lineIndex = index
    
  def get_lineIndex(self):
    return self.lineIndex

  def isLastLine(self):
    return self.lineIndex == len(self.data) - 1

  def set_lineIndexToStart(self):
    self.lineIndex = 0

class ComponentStackList:

  def __init__(self):
    self.data = []
    self.fileHandle = open(PCBdataDir + 'componentStacks.csv', 'rb')
    self.reader = csv.reader(self.fileHandle, delimiter=',', quotechar='"')
    for row in self.reader:
      # throw comments away
      if (len(row[0]) == 0) or (row[0][0] in ["#", "%"]): continue # skip comments and unused component stacks

      # treat empty cases as zero
      for i in [4, 5, 6, 7]:
        if row[i] == '': row[i] = 0.0

      # set feedrate to common 4mm per component if empty
      if row[2] == '': row[2] = '4'

      # set speed to 100 % if empty
      if row[8] == '': row[8] = '100'

      self.data.append(
             dict(stockId        = row[0],  # stockIds are strings not integers
                  stackNo        = int(row[1]),
                  feedRate       = int(row[2]),
                  head           = int(row[3]),
                  height         = float(row[4]),
                  rotationOffset = float(row[5]),
                  xOffset        = float(row[6]),
                  yOffset        = float(row[7]),
                  speed          = int(row[8])
                  ))


  def get_data(self):
    return self.data
    
def drawRotatedLine(pixmap, gc, x1, y1, x2, y2, angle, centerX, centerY):
  x1r = ((x1-centerX) * math.cos(angle)) - ((y1-centerY) * math.sin(angle))
  y1r = ((x1-centerX) * math.sin(angle)) + ((y1-centerY) * math.cos(angle))
  x2r = ((x2-centerX) * math.cos(angle)) - ((y2-centerY) * math.sin(angle))
  y2r = ((x2-centerX) * math.sin(angle)) + ((y2-centerY) * math.cos(angle))
  pixmap.draw_line(gc, centerX+int(x1r), centerY+int(y1r), centerX+int(x2r), centerY+int(y2r))

    
class  chipShooterApp:

    def __init__(self, bom, coordinates, boardsize, componentStackList):
	filename = "chipShooter.glade.xml"
	self.builder = gtk.Builder()
	self.builder.add_from_file(filename)
	self.window = self.builder.get_object("window")
	self.builder.connect_signals(self)

        self.viewport1 = self.builder.get_object("viewport1")
        print "upper, lower:", self.viewport1.get_hadjustment().get_upper(), self.viewport1.get_hadjustment().get_lower()
        print "upper, lower:", self.viewport1.get_hadjustment().get_upper(), self.viewport1.get_hadjustment().get_lower()
        
        self.image = self.builder.get_object("image")
        
        self.pixbufTop    = gtk.gdk.pixbuf_new_from_file(PCBdataDir + "board.top.jpg")
        self.pixbufBottom = gtk.gdk.pixbuf_new_from_file(PCBdataDir + "board.bottom.jpg").flip(horizontal=0).flip(horizontal=1) # flipped vertically
        self.pixbuf       = self.pixbufTop
     
        self.croshX = 0.3
        self.croshY = 0.3	
        self.scale = 0.15
        
        self.bom = bom
        self.coordinates = coordinates
        self.boardsize = boardsize.get_size()
        self.componentStackList = componentStackList
        self.show_line(bom.get_line())

        self.rescaleImage()
        
    def combinedLineAndComponentStackDict (self, line):
      # join data from coordinates and componentStack to get more information about the component line data
      result = line.copy()
      if line['refdes'] in self.coordinates.get_data().keys():
        result['rotation']     = self.coordinates.get_data()[line['refdes']]['rotation']
        result['mountingSide'] = str(self.coordinates.get_data()[line['refdes']]['mountingSide'])
        result['xCoord']       = self.coordinates.get_data()[line['refdes']]['x']
        result['yCoord']       = self.coordinates.get_data()[line['refdes']]['y']

      for componentStack in self.componentStackList.get_data():
        if componentStack['stockId'] == line['stockId']:
           result['rotationOffset'] = componentStack['rotationOffset']
           result['stackNo'] = componentStack['stackNo']
           result['feedRate'] = componentStack['feedRate']
           result['head'] = componentStack['head']
           result['height'] = componentStack['height']
           result['rotationOffset'] = componentStack['rotationOffset']
           result['xOffset'] = componentStack['xOffset']
           result['yOffset'] = componentStack['yOffset']
           result['speed'] = componentStack['speed']
           break

      if result.has_key('rotation') and result.has_key('rotationOffset') :
        result['totalRotation'] = result['rotation'] + result['rotationOffset']
        while result['totalRotation'] >  180 : result['totalRotation'] -= 360
        while result['totalRotation'] < -180 : result['totalRotation'] += 360

      return result


    def show_line(self, line):
   
        combinedLine = self.combinedLineAndComponentStackDict(line)

        self.builder.get_object("lineNumber").set_label(str(combinedLine['lineNumber']))
        self.builder.get_object("quantity").set_label(str(combinedLine['position']+1)+'/'+str(combinedLine['quantity']))
        self.builder.get_object("footprint").set_label(str(combinedLine['footprint']))
        self.builder.get_object("value").set_label(str(combinedLine['value']))
        self.builder.get_object("stockId").set_label(str(combinedLine['stockId']))
        self.builder.get_object("refdes").set_label(str(combinedLine['refdes']))
        self.builder.get_object("skip").set_label(str(combinedLine['skip']))
        self.builder.get_object("comment").set_label(str(combinedLine['comment']))
        if combinedLine.has_key('rotation'):
          self.builder.get_object("rotation").set_label(str(combinedLine['rotation']))
        else: self.builder.get_object("rotation").set_label('undefined')
        if combinedLine.has_key('mountingSide'):
          self.builder.get_object("mountingSide").set_label(combinedLine['mountingSide'])
        else: self.builder.get_object("mountingSide").set_label('undefined')

        if combinedLine.has_key('xCoord'):
          self.croshX = combinedLine['xCoord']/(self.boardsize['x'])
          # board coordinates are zero at bottom, gtk coordinates are zero at top : reverse them
          self.croshY = 1.0-(combinedLine['yCoord']/(self.boardsize['y']))
        else:
          self.croshX = None
          self.croshY = None
          
        # top or bottom ?
        if combinedLine.has_key('mountingSide'):
          top = combinedLine['mountingSide'] == 'top'
        else:
          top = None
      
        if top or (top is None):
          self.pixbuf = self.pixbufTop
        else:
          self.pixbuf = self.pixbufBottom

        if combinedLine.has_key('stackNo'):
          self.builder.get_object("stackNo").set_label(str(combinedLine['stackNo']))
          self.builder.get_object("feedRate").set_label(str(combinedLine['feedRate']))
          self.builder.get_object("head").set_label(str(combinedLine['head']))
          self.builder.get_object("height").set_label(str(combinedLine['height']))
          self.builder.get_object("rotationOffset").set_label(str(combinedLine['rotationOffset']))
          self.builder.get_object("xOffset").set_label(str(combinedLine['xOffset']))
          self.builder.get_object("yOffset").set_label(str(combinedLine['yOffset']))
          self.builder.get_object("speed").set_label(str(combinedLine['speed']))            
        else:
          # loop fell through without finding an associated component stack     
          self.builder.get_object("stackNo").set_label('undefined')
          self.builder.get_object("feedRate").set_label('-')
          self.builder.get_object("head").set_label('-')
          self.builder.get_object("height").set_label('-')
          self.builder.get_object("rotationOffset").set_label('-')
          self.builder.get_object("xOffset").set_label('-')
          self.builder.get_object("yOffset").set_label('-')
          self.builder.get_object("speed").set_label('-')

        if combinedLine.has_key('totalRotation'): self.totalRotation = combinedLine['totalRotation']
        else: self.totalRotation = None

        self.rescaleImage()

    def generateTM240Afile(self, mountingSide):
    # generates pick and place file for top or bottom face of the PCB

        # open output file
        file = open(PCBdataDir + 'TM240Acontrol.generated.' + mountingSide + '.csv', 'w')
  
        # copy 65535,0 section template file to output
        header = open(PCBdataDir + 'pnp.TM240A.template.0.csv', 'rb') 
        file.write(header.read()) 
        header.close()

        # generate stack offset lines
        for componentStack in self.componentStackList.get_data():
          file.write('65535, 1, ' + str(componentStack['stackNo'])  + ', ' 
                                  + str(componentStack['xOffset'])  + ', ' 
                                  + str(componentStack['yOffset'])  + ',\n')

        # generate stack feed speed lines
        for componentStack in self.componentStackList.get_data():
          file.write('65535, 2, ' + str(componentStack['stackNo']) + ', ' + str(componentStack['feedRate']) + '\n')

        # copy section 65535,3 section template file to output
        header = open(PCBdataDir + 'pnp.TM240A.template.3.csv', 'rb') 
        file.write(header.read()) 
        header.close()

        self.bom.set_lineIndexToStart()
        outputFileLineNumber = 1
        lastSpeed = None;
        while True:
          line = self.bom.get_line()
          combinedLine = self.combinedLineAndComponentStackDict(line)

          if (combinedLine.has_key('totalRotation') 
              and combinedLine['mountingSide'] == mountingSide): 
          # totalRotation is an indication that all the data is present
            # speed command line
            if combinedLine['speed'] != lastSpeed: 
              file.write('0, ' + str(int(combinedLine['speed']/10)) + ', 0, 0, 0, 0, 0, 0\n')
              lastSpeed = combinedLine['speed']
            # repeat offset for stack 0, which is used for different components
            if combinedLine['stackNo'] == 0:
              file.write('65535, 1, ' + str(combinedLine['stackNo'])  + ', ' 
                                      + str(combinedLine['xOffset'])  + ', ' 
                                      + str(combinedLine['yOffset'])  + ',\n')

            # component placement line
            file.write(str(outputFileLineNumber)    + ', ' +
                       str(combinedLine['head']) + ', ' +
                       str(combinedLine['stackNo']) + ', ' +
                       "{:.2f}".format(combinedLine['xCoord'])+ ', ' +
                       "{:.2f}".format(combinedLine['yCoord'])+ ', ' +
                       str(int(combinedLine['totalRotation'])) + ', ' +
                       str(combinedLine['height']) + ', ' +
                       str(combinedLine['skip']) + ', ' +
                       str(combinedLine['refdes']) + ', ' +
                       str(combinedLine['value']) + ', ' +
                       str(combinedLine['footprint']) + ', ' +
                       str(combinedLine['comment']) + '\n')
                    
          if self.bom.isLastLine(): break
          else : 
            self.bom.set_lineIndex(self.bom.get_lineIndex()+1)
            outputFileLineNumber += 1

        file.close()

        
    def on_window_destroy(self, widget, data=None):
      gtk.main_quit()
	
    # start pan	
    def on_viewport1_button_press_event(self, widget, event, data=None):
      if event.button in [1,2]:
        print 'button ', event.button
	self.panStartX = event.x
	self.panStartY = event.y
	self.hadjustmentStart = self.viewport1.get_hadjustment().get_value()	
	self.vadjustmentStart = self.viewport1.get_vadjustment().get_value()	
        
    # pan
    def on_viewport1_motion_notify_event(self, widget, event):	
      print "viewport :"
      if event.state & gtk.gdk.BUTTON2_MASK:
	print "  panned by:", event.x - self.panStartX, event.y - self.panStartY
        print "hadjustement", self.viewport1.get_hadjustment().get_value()
        print "upper, lower:", self.viewport1.get_hadjustment().get_upper(), self.viewport1.get_hadjustment().get_lower()
	self.viewport1.get_hadjustment().set_value(- event.x + self.panStartX + self.hadjustmentStart)
	self.viewport1.get_vadjustment().set_value(- event.y + self.panStartY + self.vadjustmentStart)
	#self.viewport1.get_hadjustment().value_changed()
	#self.viewport1.get_vadjustment().value_changed()
	
      return self.on_image_motion_notify_event(widget, event)
      
    def on_window_key_press_event(self, widget, event):
      try : print chr(event.keyval), event.keyval
      except Exception: print event.keyval

      # left arrow
      if event.keyval in [65430, 65361]: pass
      # up arrow
      if event.keyval in [65431, 65362]: self.bom.set_lineIndex(self.bom.get_lineIndex()-1)
      # right arrow
      if event.keyval in [65432, 65363]: pass
      # down arrow
      if event.keyval in [65433, 65364]: self.bom.set_lineIndex(self.bom.get_lineIndex()+1)
      # page up
      if event.keyval in [65434, 65365]: self.bom.set_lineIndex(self.bom.get_lineIndex()-10)
      # page down
      if event.keyval in [65435, 65366]: self.bom.set_lineIndex(self.bom.get_lineIndex()+10)
      # + 
      if event.keyval in [65451, 43]: self.scale *= 1.2; self.rescaleImage()
      # - 
      if event.keyval in [65453, 45]: self.scale /= 1.2; self.rescaleImage()
      # q Q
      if event.keyval in [ord('q'), ord('Q')]: exit()

      self.show_line(self.bom.get_line())
      self.rescaleImage()
     
    def drawCrosshair(self):
      self.pixmap, self.mask = self.scaledPixbuf.render_pixmap_and_mask()
      if not (self.croshX is None):
      	x = int(self.pixmap.get_size()[0]*self.croshX)
      	y = int(self.pixmap.get_size()[1]*self.croshY)
        cm = self.pixmap.get_colormap()
        red = cm.alloc_color('red')
        gc = self.pixmap.new_gc(foreground=red)
        self.pixmap.draw_line(gc, x, 0, x,self.pixmap.get_size()[1])
        self.pixmap.draw_line(gc, 0, y,self.pixmap.get_size()[0], y)
        size = 30
        self.pixmap.draw_arc(gc, False, x-size/2, y-size/2, size, size, 0, 360 * 64)
        size = 60
        self.pixmap.draw_arc(gc, False, x-size/2, y-size/2, size, size, 0, 360 * 64)
        size = 90
        self.pixmap.draw_arc(gc, False, x-size/2, y-size/2, size, size, 0, 360 * 64)
        size = 92
        self.pixmap.draw_arc(gc, False, x-size/2, y-size/2, size, size, 0, 360 * 64)
        size = 94
        self.pixmap.draw_arc(gc, False, x-size/2, y-size/2, size, size, 0, 360 * 64)

        if not (self.totalRotation is None):
          angle = math.radians(self.totalRotation)
          cm = self.pixmap.get_colormap()
          blue = cm.alloc_color('blue')
          gc = self.pixmap.new_gc(foreground=blue)
          size = 30
          drawRotatedLine(self.pixmap, gc, x-size/2, y+size/2, x, y-size/2, angle, x, y)
          drawRotatedLine(self.pixmap, gc, x, y-size/2, x+size/2, y+size/2, angle, x, y)
          size = 60
          drawRotatedLine(self.pixmap, gc, x-size/2, y+size/2, x, y-size/2, angle, x, y)
          drawRotatedLine(self.pixmap, gc, x, y-size/2, x+size/2, y+size/2, angle, x, y)
          size = 90
          drawRotatedLine(self.pixmap, gc, x-size/2, y+size/2, x, y-size/2, angle, x, y)
          drawRotatedLine(self.pixmap, gc, x, y-size/2, x+size/2, y+size/2, angle, x, y)


    def rescaleImage(self):
	self.scaledPixbuf = self.pixbuf.scale_simple(int(self.pixbuf.get_width() * self.scale), int(self.pixbuf.get_height() * self.scale), gtk.gdk.INTERP_BILINEAR)
	#self.image.set_from_pixbuf(pixbuf)
	
        self.drawCrosshair()
        self.image.set_from_pixmap(self.pixmap, self.mask)


    def on_image_scroll_event(self, widget, event):
      print "image scroll event"
      if event.direction == gtk.gdk.SCROLL_UP:
	print "You scrolled up"
      self.rescaleImage()
      return True
	
    def on_image_motion_notify_event(self, widget, event):
      #print "Mouse moved to", event.x, event.y
      return True

    def on_scrolledwindow1_motion_notify_event(self, widget, event):	
      print "scrolledwindow1 :"
      return self.on_image_motion_notify_event(widget, event)
    
    def on_window_motion_notify_event(self, widget, event):	
      # print "window :"
      return self.on_image_motion_notify_event(widget, event)
      
    def on_eventbox1_scroll_event(self, widget, event):
      print "eventbox1 scroll event"
          
    def on_viewport1_scroll_event(self, widget, event):
      print "viewport scroll event"
              
    def on_scrolledwindow1_scroll_event(self, widget, event):
      print "on_scrolledwindow1_scroll_event scroll event"
              
    def on_window_scroll_event(self, widget, event):
      print "window scroll event"
      if event.direction == gtk.gdk.SCROLL_UP:
	print "You scrolled up"
	self.scale *= 1.2
      if event.direction == gtk.gdk.SCROLL_DOWN:
	print "You scrolled down"
	self.scale /= 1.2

      if self.scale > 3:
        self.scale = 3
      if self.scale < 0.1:
        self.scale = 0.1

      self.rescaleImage()
      return True

    def on_viewport1_scroll_event(self, widget, event):
	self.on_window_scroll_event(widget, event);
    
    def on_generateButton_clicked_event(self, event):
        self.generateTM240Afile(mountingSide='top')
        self.generateTM240Afile(mountingSide='bottom')

if __name__ == "__main__":
  size = Boardsize()
  bom = Bom()
  coordinates = Coordinates(size)
  componentStackList = ComponentStackList()
  app = chipShooterApp(bom, coordinates, size, componentStackList)
  app.window.show()
  gtk.main()

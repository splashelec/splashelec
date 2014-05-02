#!/usr/bin/env python

import gtk
import csv
import math

PCBdataDir = 'exampleBoardData'
if PCBdataDir[len(PCBdataDir)-1] != '/':
   PCBdataDir = PCBdataDir + '/';

class Boardsize:
  def __init__(self):
    self.reader = csv.reader(open(PCBdataDir + 'board.size', 'rb'), delimiter=',', quotechar='"')
    for row in self.reader:
      # throw comments away
      if row[0][0] == "#": continue
      self.x = float(row[0])
      self.y = float(row[1])
      
  def get_size(self):
    return dict(x=self.x, y=self.y)


class Coordinates:
  def __init__(self):
    self.data = {}
    self.reader = csv.reader(open(PCBdataDir + 'board.xy', 'rb'), delimiter=',', quotechar='"')
    for row in self.reader:
      # throw comments away
      if row[0][0] == "#": continue
      refdes       = row[0]
      x            = float(row[3])
      y            = float(row[4])
      rotation     = float(row[5])
      mountingSide = row[6]
      # create a dictionary that associates the refdes to the coordinates
      self.data[refdes] = dict(x=x, y=y, 
                               rotation=rotation, 
                               mountingSide=mountingSide)

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

class ComponentStackList:

  def __init__(self):
    self.data = []
    self.fileHandle = open(PCBdataDir + 'pnp.componentStacks.csv', 'rb')
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
        
    def show_line(self, line):
        if line['refdes'] in self.coordinates.get_data().keys():
          rotation     = self.coordinates.get_data()[line['refdes']]['rotation']
          mountingSide = str(self.coordinates.get_data()[line['refdes']]['mountingSide'])
          xCoord       = self.coordinates.get_data()[line['refdes']]['x']
          yCoord       = self.coordinates.get_data()[line['refdes']]['y']
        else:
          rotation     = None
          mountingSide = None
          xCoord       = None
          yCoord       = None

        self.builder.get_object("lineNumber").set_label(str(line['lineNumber']))
        self.builder.get_object("quantity").set_label(str(line['position']+1)+'/'+str(line['quantity']))
        self.builder.get_object("footprint").set_label(str(line['footprint']))
        self.builder.get_object("value").set_label(str(line['value']))
        self.builder.get_object("stockId").set_label(str(line['stockId']))
        self.builder.get_object("refdes").set_label(str(line['refdes']))
        self.builder.get_object("skip").set_label(str(line['skip']))
        self.builder.get_object("comment").set_label(str(line['comment']))
        if not rotation is None:
          self.builder.get_object("rotation").set_label(str(rotation))
        else: self.builder.get_object("rotation").set_label('undefined')
        if not mountingSide is None:
          self.builder.get_object("mountingSide").set_label(mountingSide)
        else: self.builder.get_object("mountingSide").set_label('undefined')

        if not (xCoord is None):
          self.croshX = xCoord/(self.boardsize['x'])
          self.croshY = 1.0-(yCoord/(self.boardsize['y']))
        else:
          self.croshX = None
          self.croshY = None
          
        # top or bottom ?
        if not mountingSide is None:
          top = self.coordinates.get_data()[line['refdes']]['mountingSide'] == 'top'
        else:
          top = None
      
        if top or (top is None):
          self.pixbuf = self.pixbufTop
        else:
          self.pixbuf = self.pixbufBottom
          self.croshX = 1.0 - self.croshX # mirror

        for componentStack in self.componentStackList.get_data():
          if componentStack['stockId'] == line['stockId']:
             rotationOffset = componentStack['rotationOffset']
             self.builder.get_object("stackNo").set_label(str(componentStack['stackNo']))
             self.builder.get_object("feedRate").set_label(str(componentStack['feedRate']))
             self.builder.get_object("head").set_label(str(componentStack['head']))
             self.builder.get_object("height").set_label(str(componentStack['height']))
             self.builder.get_object("rotationOffset").set_label(str(componentStack['rotationOffset']))
             self.builder.get_object("xOffset").set_label(str(componentStack['xOffset']))
             self.builder.get_object("yOffset").set_label(str(componentStack['yOffset']))
             self.builder.get_object("speed").set_label(str(componentStack['speed']))
             break
        else:
          # loop fell through without finding an associated component stack     
          rotationOffset = None
          self.builder.get_object("stackNo").set_label('undefined')
          self.builder.get_object("feedRate").set_label('-')
          self.builder.get_object("head").set_label('-')
          self.builder.get_object("height").set_label('-')
          self.builder.get_object("rotationOffset").set_label('-')
          self.builder.get_object("xOffset").set_label('-')
          self.builder.get_object("yOffset").set_label('-')
          self.builder.get_object("speed").set_label('-')

        if (rotation is None) or (rotationOffset is None) : self.rotation = None
        else: self.rotation = rotation + rotationOffset

        self.rescaleImage()
        
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
      if event.keyval in [65431, 65362]: bom.set_lineIndex(bom.get_lineIndex()-1)
      # right arrow
      if event.keyval in [65432, 65363]: pass
      # down arrow
      if event.keyval in [65433, 65364]: bom.set_lineIndex(bom.get_lineIndex()+1)
      # page up
      if event.keyval in [65434, 65365]: bom.set_lineIndex(bom.get_lineIndex()-10)
      # page down
      if event.keyval in [65435, 65366]: bom.set_lineIndex(bom.get_lineIndex()+10)
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

        if not (self.rotation is None):
          angle = math.radians(self.rotation)
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
      print "Mouse moved to", event.x, event.y
      return True

    def on_scrolledwindow1_motion_notify_event(self, widget, event):	
      print "scrolledwindow1 :"
      return self.on_image_motion_notify_event(widget, event)
    
    def on_window_motion_notify_event(self, widget, event):	
      print "window :"
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
    
if __name__ == "__main__":
  size = Boardsize()
  bom = Bom()
  coordinates = Coordinates()
  componentStackList = ComponentStackList()
  app = chipShooterApp(bom, coordinates, size, componentStackList)
  app.window.show()
  gtk.main()

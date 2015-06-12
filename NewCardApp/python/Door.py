#!/usr/bin/env python      
import Tkinter as tk       
import urllib
import urllib2
import re

class Application(tk.Frame):              
    def __init__(self, master=None):
        tk.Frame.__init__(self, master)   
        self.grid()                       
        self.createWidgets()
	#self.getData()
	self.Connect2Web()

    def Connect2Web(self):
	aResp = urllib2.urlopen("http://www.brianmichael.org/current.php");
	web_pg = aResp.read();
	#print web_pg
	pattern = "<tr id='.*?'><td id='id'>.*?</td><td id='tag'>(.*?)</td>.*</tr>"
        p = re.compile(pattern)
        m = p.match(web_pg)
        if m:
          print "\tID:", m.group(1)
          #print "\tTag:", m.group(2)
          #print "\tFirst Name:", m.group(3)
          #print "\tLast Name:", m.group(4)
          #print "\tMain:", m.group(5)
          #print "\tKitchen:", m.group(6)
          #print "\tTool Room:", m.group(7)
          #print "\tSimulator:", m.group(8)
          #print "\tStorage:", m.group(9)
        else:
          print "Nothing found"

    def createWidgets(self):
	membersLabelTxt = tk.StringVar()
        membersLabelTxt.set('Members:')
	self.usersLabel = tk.Label(self, textvariable=membersLabelTxt)
	self.usersLabel.grid()

	#self.yScroll = tk.Scrollbar(self, orient=tk.VERTICAL)
	#self.yScroll.grid(row=0, column=1, sticky=tk.N+tk.S)

	#self.xScroll = tk.Scrollbar(self, orient=tk.HORIZONTAL)
	#self.xScroll.grid(row=1, column=0, sticky=tk.E+tk.W)

	self.listbox = tk.Listbox(self, selectmode=tk.SINGLE) #, xscrollcommand=self.xScroll.set, yscrollcommand=self.yScroll.set)
	#self.listbox.grid(row=0, column=0, sticky=tk.N+tk.S+tk.E+tk.W)
	#self.xScroll['command'] = self.listbox.xview
	#self.yScroll['command'] = self.listbox.yview
	self.listbox.grid()

        self.quitButton = tk.Button(self, text='Quit', command=self.quit)            
        self.quitButton.grid()            

app = Application()                       
app.master.title('EAA690 RFID Door') 
app.mainloop()  

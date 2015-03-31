#!/usr/bin/env python      
import Tkinter as tk       

class Application(tk.Frame):              
    def __init__(self, master=None):
        tk.Frame.__init__(self, master)   
        self.grid()                       
        self.createWidgets()

    def createWidgets(self):
	self.yScroll = tk.Scrollbar(self, orient=tk.VERTICAL)
	self.yScroll.grid(row=0, column=1, sticky=tk.N+tk.S)

	self.xScroll = tk.Scrollbar(self, orient=tk.HORIZONTAL)
	self.xScroll.grid(row=1, column=0, sticky=tk.E+tk.W)

	self.listbox = tk.Listbox(self, selectmode=tk.SINGLE, xscrollcommand=self.xScroll.set, yscrollcommand=self.yScroll.set)
	self.listbox.grid(row=0, column=0, sticky=tk.N+tk.S+tk.E+tk.W)
	self.xScroll['command'] = self.listbox.xview
	self.yScroll['command'] = self.listbox.yview
	self.listbox.grid()

        self.quitButton = tk.Button(self, text='Quit', command=self.quit)            
        self.quitButton.grid()            

app = Application()                       
app.master.title('Sample application') 
app.mainloop()  

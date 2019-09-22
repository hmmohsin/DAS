import socket
import os
import sys

class config():
	def __init__(self,configFile):
		self.configFile=configFile
		self.config={}

	def load_config(self):
		configFileHandle= open(self.configFile,"r")
		for config in configFileHandle:
			key, value = config.rstrip().split(' ')
			if (key == "lport"):
				self.config['lport'] = int(value)
			if (key == "ipAddr"):
				self.config['ipAddr'] = value
			if (key == "nnPort"):
				self.config['nnPort'] = int(value)
			if (key == "nnIPAddr"):
				self.config['nnIPAddr'] = value
			if (key == "HDFSDataDir"):
				self.config['HDFSDataDir'] = value

	def get_config(self, key):
		return self.config[key]

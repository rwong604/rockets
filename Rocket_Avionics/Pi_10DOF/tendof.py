from L3GD20 import L3GD20
import smbus
import time

"""
	Class to handle all interactions with the 10DOF. 
"""
class Unified_Sensor: 
	"""
		Initialize all sensors via i2c.  
	"""
	def __init__(self): 
		self.L3GD20 = L3GD20(busId = 1, slaveAddr = 0x6b, ifLog = False, ifWriteBlock=False)
		self.L3GD20.Set_PowerMode("Normal")
		self.L3GD20.Set_FullScale_Value("250dps")
		self.L3GD20.Set_AxisX_Enabled(True)
		self.L3GD20.Set_AxisY_Enabled(True)
		self.L3GD20.Set_AxisZ_Enabled(True)
		self.L3GD20.Init()
		self.L3GD20.Calibrate()

		self.bmp = BMP085.BMP085()
		self.lsm = Adafruit_LSM303()

		# for sensing roll rate
		self.start_time = time.time()
		self.dt = 0.02
		self.x = 0 
		self.y = 0
		self.z = 0

	"""
		BMP pressure. 
	"""
	def get_pressure(self):
		return self.bmp.read_pressure()

	"""
		BMP temperature. 
	"""
	def get_temperature(self):
		return self.bmp.read_temperature()

	"""
		BMP altitude. 
	"""
	def get_altitude(self):
		return self.bmp.read_altitude()

	"""
		L3GD20 xyz change. 
	"""
	def get_xyz(self):
		xyz_1 = s.Get_CalOut_Value()
		time.sleep(self.dt) # sleep a bit so we can get the difference in xyz
		xyz_2 = s.Get_CalOut_Value() 

		self.x += (dxyz_2[0] - dxyz_1[0])*dt;
		self.y += (dxyz_2[1] - dxyz_1[1])*dt;
		self.z += (dxyz_2[2] - dxyz_1[2])*dt;
		return (self.x, self.y, self.z)

	"""
		TODO: the LSM data needs to be parsed. 
	"""
	def get_lsm(self):
		return self.lsm.read()

	def __repr__(self):
		return "I am a unified sensor. "

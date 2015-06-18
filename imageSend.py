#
import serial, time, os, sys
from subprocess import call, Popen

#Set serial baud rate
baud = 1500000
flag = 0
dropped = 0
#Move into the imageConv directory
os.chdir('./imageConv')
#Delete all existing pnm images
for file in os.listdir("./"):
        if file.endswith(".pgm"):
                os.remove(file)
#Convert the specified input file
if sys.argv[1] == 'f':
    ret =call('ffmpeg -i '+sys.argv[2]+' -s 128x96 -r 11.7 -threads 8 -loglevel panic out%05d.pgm')
elif sys.argv[1] == 'c':
    Popen('ffmpeg -f dshow -i video="FaceTime HD Camera (Built-in)" -s 128x96  -loglevel panic -updatefirst 1 out%05d.pgm')
    ret =0
#Check if the conversion was successful
if ret ==0 :
        #Start the serial connection
        ser = serial.Serial('COM9',baud)
        #Blank the write array
        toWrite = ''
        #Catch keyboard interrupts
        try:
                #Loop until interrupt
                while 1:
                        if sys.argv[1] == 'f':
                            #Run a player to view video output/ play sound if it exists
                            Popen('ffplay -autoexit -loglevel panic '+sys.argv[2])
                        #Loop through all pnm files in directory
                        for file in os.listdir("./"):
                                if file.endswith(".pgm"):
                                        #Open the file
                                        f = open(file,'rb')
                                        for x in range(0,3):
                                                f.readline()
                                        #Send start byte
                                        toWrite+=chr(0xFF)
                                        #Loop though each pixel
                                        for x in range(0,96):
                                                for y in range(0,64):
                                                        #Read pixel A [l]
                                                        pixA = f.read(1)
                                                        #Detect dropped frame
                                                        if len(pixA)>0:
                                                            l = ord(pixA)
                                                        else:
                                                            flag =1
                                                            break
                                                        #Set pixel A to luminence value
                                                        nibH = int(l)
                                                        #Read pixel B [l]
                                                        pixB = f.read(1)
                                                        #Detect dropped frame
                                                        if len(pixB)>0:
                                                            l = ord(pixB)
                                                        else:
                                                            flag =1
                                                            break
                                                        #Set pixel B to luminence value
                                                        nibL = int(l)
                                                        #Combine nibles to form 2 pixel byte
                                                        byte = nibH &0xf0 | (nibL >>4)
                                                        #Remove start byte if it occurs
                                                        if byte ==0xFF:
                                                                byte = 0xEE
                                                        #Add byte to serial array
                                                        toWrite+=chr(byte)
                                                        if flag:
                                                            break
                                                if flag:
                                                    flag =0
                                                    f.close()
                                                    dropped +=1
                                                    break
                                        #Send the serial array
                                        ser.write(toWrite)
                                        #Blank the serial array
                                        toWrite = ''
                                        #Close the file
                                        f.close()
        #If a keyboard interrupt occurs, handle it
        except KeyboardInterrupt:
                #Close the open file
                f.close()
                #Blank the serial array
                toWrite = ''
                #Add start byte
                toWrite+=chr(0xFF)
                #Create black screen
                for x in range(0,96):
                        for y in range(0,64):
                                toWrite+=chr(0)
                #Send black screen
                ser.write(toWrite)
                #Create black screen
                for x in range(0,96):
                        for y in range(0,64):
                                toWrite+=chr(0)
                #Send black screen
                ser.write(toWrite)
                #Create black screen
                for x in range(0,96):
                        for y in range(0,64):
                                toWrite+=chr(0)
                #Send black screen
                ser.write(toWrite)

        #Close the serial connection
        ser.close()
        #Print exit message
        print('Frames dropped:'+str(dropped))
        print('Closed -- exit')
#If the conversion was not successful, exit and print an error
else:
      print('Input file error')

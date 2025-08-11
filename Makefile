all:	
	cd client && $(MAKE)
	cd server && $(MAKE)

clean:
	cd client && $(MAKE) clean
	cd server && $(MAKE) clean
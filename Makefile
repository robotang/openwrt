all:
	@echo
	@echo "options:"
	@echo "   make download - check out openwrt svn trunk (revision 30753)"
	@echo "   make symlinks - make symlinks that are needed to build kernel modules (should be done after openwrt has been made"
	
download:
	cd ~/dev && mkdir openwrt && cd openwrt && 	svn co -r 30753 svn://svn.openwrt.org/openwrt/trunk/
	
symlinks:
	

.PHONY:
	download symlinks

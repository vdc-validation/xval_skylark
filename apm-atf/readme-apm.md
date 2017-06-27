1. Where to place your code
a. Board related:
- plat/apm/xgene/board/common/
  Common code shared between all boards.

- plat/apm/xgene/board/<board name>/<board name>_setup.c
  Board initialization such as GPIO, etc. The function xgene_board_init will be called early by BL1.

- plat/apm/xgene/board/<board name>/<board_name>_err.c.
  Handle platform error condition such as BL1 fail to authenticate images.

- plat/apm/xgene/include/board/<board name>/
  Header files for board definition.

b. Chip related:
- plat/apm/xgene/include/<chip name>/xgene_def.h
  Chip definition.

- plat/apm/xgene/soc/<chip name>/drivers/
  Specific chip drivers.

- plat/apm/xgene/include/<chip_name>/
  Header file for chip drivers.

c. Common things:
- plat/apm/xgene/common/
- plat/apm/xgene/drivers/
  Common drivers, subsystem such as PSCI, GIC.

- plat/apm/xgene/include/
  Common header files.

2. Build
- Export CROSS_COMPILE
- Build release for run on VHP:
make PLAT=xgene BUILD_STRING=test XGENE_VHP=1 TARGET_BOARD=eagle all BL33=<point to Tianocore binary> fip;

- Build debug for run on VHP:
make PLAT=xgene DEBUG=1 LOG_LEVEL=50 BUILD_STRING=test XGENE_VHP=1 TARGET_BOARD=eagle all BL33=<point to Tianocore binary> fip;

- Build release with trusted board boot (need to install openssl, crypt library on build machine). 
  Download mbedtls source code (tested with 2.3.0).
  For developing, we use ARM's ROT key, OID definition. For release, need to use APM's reserved values.
make PLAT=xgene GENERATE_COT=1 MBEDTLS_DIR=<path to mbedtls source code> XGENE_ROTPK_LOCATION=devel_rsa TRUSTED_BOARD_BOOT=1 ROT_KEY=plat/arm/board/common/rotpk/arm_rotprivk_rsa.pem BUILD_STRING=test TARGET_BOARD=eagle BL33=<point to Tianocore binary> all fip;

3. Commit to git.
- Generate patch, check patch and send email for review to tphan@apm.com, lho@apm.com and other people related.

4. Run ATF
a. Run on VHP:
- Use the latest VHP: /projects/svdc/mat/skylark/skylark-vhp-1.2.2-2016-07-11.tar.bz2
- Untar the tar file and copy doc/skylarkcfg.xml to VHP folder.
- Modify skylarkcfg.xml to enable UART4.
- Copy //processor/skylark/tools/vhp/jump.o to 0x0 (it will jump to address 0x1d000000).
- Copy bl1.bin to 0x1d000000
- Copy fip.bin to 0x81200000(will change).

- Example: .amccsimrc
	sim start

	## Suppress log messages
	::log::loglevel  7
	## Load flash image
	::sim::loadImage FLASH /home/tphan/jump.o 0x0 0x1000
	::sim::loadImage OCM  /home/tphan/bl1.bin 0x0 0x20000
	::sim::loadImage DDR  /home/tphan/fip.bin 0x81200000 0xE00000

	## Start cores
	::sim::startpmd 0x3

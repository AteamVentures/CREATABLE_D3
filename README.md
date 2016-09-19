# CREATABLE_D3
The Perfect 3D Printer for Beginners, Educators and Experts.
[creatablelabs.com](http://creatablelabs.com/)

![image](http://ateamventures.com/wp2/wp-content/themes/ateamventures/img/intro/intro3-1.png)
####폴더 설명
 - Hardware : Mainboard, OLED board, Heat bed 회도로 및 PCB 파일 (Mainboard는 Cadence의 Allegro로 작업이되어 있고 나머지는 Eagle CAD로 작업)
 - Firmware : Mainboard firmware 
	 - File Path : CREATABLE_D3/Firmware/D3_Firmware_Ver1.1.0_160825.hex
	 
	    1. USB 케이블로 프린터와 컴퓨터를 연결합니다.
	    2. 큐라를 실행하고 메인 메뉴의 Machine > Install Custom Firmware... 를 실행합니다.
	    3. 업데이트 하려는 펌웨어 파일(확장자가 hex)를 선택하고 열기를 클릭합니다. 
	  
 - stk500v2 : 메인 보드 ATmega2560 bootloader
 
#### Creatable D2와 다른점 (H/W)
  
- USB를 위해 Silab사의 CP2104를 사용함
- 모터드라이버 A4988을 온보드함
- OLED보드: 기존 2보드를 1개의 보드로 통합함
- 히트베드: 저항값을 조정하여 빨리 가열이 되게 함

  

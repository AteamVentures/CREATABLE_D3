# CREATABLE_D3
The Perfect 3D Printer for Beginners, Educators and Experts.
[creatablelabs.com](http://creatablelabs.com/)

![image](http://ateamventures.com/wp2/wp-content/themes/ateamventures/img/intro/intro3-1.png)
####폴더 설명
 - Hardware : Mainboard, OLED board, Heat bed 회도로 및 PCB 파일 (Mainboard는 Cadence의 Allegro로 작업이되어 있고 나머지는 Eagle CAD로 작업)
 - Firmware : Mainboard firmware [Download](https://github.com/AteamVentures/CREATABLE_D3/raw/master/Firmware/D3_Firmware_Ver1.1.0_160825.zip)  
	1. USB 케이블로 프린터와 컴퓨터를 연결합니다.
	2. 위 링크에서 받은 파일의 압축을 풉니다.
	3. 큐라를 실행하고 메인 메뉴의 Machine > Install Custom Firmware... 를 실행합니다.
	4. 2번 과정에서 나온 Hex 파일을 선택하고 열기를 클립합니다.
	5. 업로딩이 끝날 때 까지 기다립니다.
	6. 업로딩 완료 후 프린터의 메뉴를 조작하여 INFO/SETTINGS > RESTORE DEFAULT 를 실행합니다.
	7. 실행 후 First run 안내에 따라 레벨링 및 필라멘트 삽입을 해 줍니다. 기존에 완료 되어있다면 추가적인 작업없이 버튼을 계속 눌러 넘어가줍니다.
	8. INFO/SETTINGS > FIRMWARE VERSION을 실행하여 버전정보를 확인합니다.
	  
 - stk500v2 : 메인 보드 ATmega2560 bootloader
 
#### Creatable D2와 다른점 (H/W)
  
- USB를 위해 Silab사의 CP2104를 사용함
- 모터드라이버 A4988을 온보드함
- OLED보드: 기존 2보드를 1개의 보드로 통합함
- 히트베드: 저항값을 조정하여 빨리 가열이 되게 함

  

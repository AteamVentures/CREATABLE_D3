# CREATABLE_D3
The Perfect 3D Printer for Beginners, Educators and Experts.
[creatablelabs.com](http://creatablelabs.com/)

![image](http://ateamventures.com/wp2/wp-content/themes/ateamventures/img/intro/intro3-1.png)
####폴더 설명
 - Hardware : Mainboard, OLED board, Heat bed 회도로 및 PCB 파일 (Mainboard는 Cadence의 Allegro로 작업이되어 있고 나머지는 Eagle CAD로 작업)
 - Firmware : Mainboard firmware 
   - File Path : CREATABLE_D3/Firmware/D3_Firmware_Ver1.0.9_160810.hex
   - Upload with CURA [참고](http://forum.creatablelabs.com/t/topic/125)
 - stk500v2 : 메인 보드 ATmega2560 bootloader
 
#### Creatable D2와 다른점 (H/W)
  
- USB를 위해 Silab사의 CP2104를 사용함
- 모터드라이버 A4988을 온보드함
- OLED보드: 기존 2보드를 1개의 보드로 통합함
- 히트베드: 저항값을 조정하여 빨리 가열이 되게 함

  

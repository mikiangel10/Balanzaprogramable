///////////////////
//version 1.0 del programa para controlar la balanza con 
///control de 4 salidas a rele y entrada de 3 celdas de carga
//Para Arduino MEGA 2560
//escrito por MIguel ANgel Gomez en 20/3/2020

#include <HX711_ADC.h>
#include <Keypad.h>
#include <Key.h>
#include <SoftPWM.h>
#include <SoftPWM_timer.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>


#define led_lcd 34
#define salida1 46
#define salida2 44
#define salida3 42  
#define salida4 40

#define MODO_MAX 1  //el numero de modo principal mayor posible

boolean listo=0;//determinar si se termino una carga o programa
int meta=0;
char motor=0;
char salida[4]={46,44,42,40};
boolean estado_salida[4]={0,0,0,0};
char modo=1;//0:modo programa,1: modo manual(MM); 2: MM-seleccionar motor; 3: MM-Cargando
char texto[4]={' ',' ',' ','0'};
char error=0;
unsigned char led_lcd_pow=50;
LiquidCrystal lcd(23,25,27,29,31,33);

///keypad variables///
boolean hold=0;
const char columnas=4;
const char filas=4;
unsigned char teclas[filas][columnas]={{'1','2','3','A'},{'4','5','6','B'},
{'7','8','9','C'},{'*','0','#','D'}};
char pinesfilas[filas]={A7,A6,A5,A4};
char pinescolumnas[columnas]={A3,A2,A1,A0};
char key=0;
Keypad Teclado(makeKeymap(teclas),pinesfilas,pinescolumnas,filas,columnas);

///load cell variables
HX711_ADC celdas[3]{HX711_ADC(50,52),HX711_ADC(30,32),HX711_ADC(26,28)};
float peso=0.0;
const char calfac_mem[3]={0xa,0xc,0xe};


void setup() {
  pinMode(led_lcd, OUTPUT);
  pinMode(salida[0], OUTPUT);
  pinMode(salida[1], OUTPUT);
  pinMode(salida[2], OUTPUT);
  pinMode(salida[3], OUTPUT);
  lcd.begin(16,2);
  
  SoftPWMBegin();  
  SoftPWMSet(led_lcd, 0);
  SoftPWMSetFadeTime(led_lcd, 1000, 1000);
  SoftPWMSet(led_lcd, led_lcd_pow);
  Teclado.addEventListener(keypadEvent);
  Teclado.setHoldTime(1000);
  Teclado.setDebounceTime(100);
  
  lcd.print("Balanzas MAYVA");
  lcd.setCursor(0,1);
  lcd.print("Iniciando...");
  
  for (int n=0;n<3;n++){
    celdas[n].begin();
    celdas[n].start(500);
    if(celdas[n].getTareTimeoutFlag()){
      lcd.setCursor(0,1);
      lcd.print(F("Falla Celda "));
      lcd.print(n+1);
      delay(5000);
      error=n+1;
    }else{
      int cf;
      EEPROM.get(calfac_mem[n], cf);
      celdas[n].setCalFactor(cf);
      lcd.setCursor(0,1);
      lcd.print("Celda ");
      lcd.print(n+1);
      lcd.print(" OK  "); 
    }
    
  }
  delay(2000);
  lcd.setCursor(0,1);
  lcd.print("          ");
  lcd.setCursor(0,0);
  
 
//     

  
}

void loop() {
  key=Teclado.getKey();

  
  switch(modo){
    case 0:  //modo programa
    
      lcd.setCursor(0,0);
      lcd.print(F("MAYVA      P:"));
      +lcd.print(texto);
      break;
 
    case 1:  //modo manual
      lcd.setCursor(0,0);
      lcd.print(F("MAYVA    Kg:     "));
      lcd.setCursor(12,0);
      lcd.print(texto);
      lcd.setCursor(0,1);
      lcd.print("Kg:");
      lcd.print(peso,2);
      
      imprimir_salidas();
      
      peso=actualizar_peso();
      
      break;

    case 21: //seleccionando motor
  
      if(key<0x40 && key>0x29){
        lcd.print(key);
        lcd.print(" ");
        motor=key;
        lcd.setCursor(6,1);
        lcd.print(motor);
      }
     
      
      imprimir_salidas();
      break;
    case 22: //cargando
    
      peso=actualizar_peso();      
      lcd.setCursor(0,0);
        if (meta>0){ 
          if(peso>=meta){
          apagar_salidas(); 
          motor=0;
          modo=1;
          break;
        }     
        lcd.print(F("Carg.M:"));
        lcd.print(motor);
        lcd.print(" Kg:");
        lcd.print(meta);
        lcd.print(F("   "));
      }else{
        lcd.print(F("Cargando manual "));
      }
      lcd.setCursor(0,1);
      lcd.print("Kg:");
      lcd.print(peso,2);
      lcd.print(" ");
      imprimir_salidas();
             
      break;
      
      
  }
  
  
}

void keypadEvent(KeypadEvent tecla){
  switch(Teclado.getState()){
    case HOLD:
      if(tecla=='*'){  ///disminuye la luminosidad del display
        led_lcd_pow=led_lcd_pow-10;
        SoftPWMSet(led_lcd, led_lcd_pow);
      }else if(tecla=='#'){  //aumenta la luminosidad del display
        led_lcd_pow=led_lcd_pow+10;  
        SoftPWMSet(led_lcd, led_lcd_pow);
      }else if(tecla=='1' && modo==21){    //seleccion de motor en modo manual
        estado_salida[0]=!estado_salida[0];
        modo=22;
      }else if(tecla=='2' && modo==21){    //seleccion de motor en modo manual
        estado_salida[1]=!estado_salida[1];
        modo=22;
      }else if(tecla=='3' && modo==21){    //seleccion de motor en modo manual
        estado_salida[2]=!estado_salida[2];
        modo=22;
      }else if(tecla=='4' && modo==21){    //seleccion de motor en modo manual
        estado_salida[3]=!estado_salida[3];
        modo=22;
      }
      hold=1;
      break;
    case RELEASED:
      if (tecla=='#' && !hold && modo<=MODO_MAX){//si se aprieta # cambia entre modo automatico y manual
        modo--;
        if(modo>MODO_MAX ){
          modo=0;
        }else if(modo<0){
          modo=MODO_MAX;
        }
      }
      
      hold=0;
      break;
    case PRESSED:
      if (tecla=='*' ){
        int n=0;
        while(texto[n]==' ' && n<sizeof(texto)-1){
          n++;
        }
        texto[n]=' ';
      }else if(tecla=='C' && (modo==22 || modo==21)){
        apagar_salidas();
        motor=0; 
        modo=1;       
      }else if(tecla=='A' ){
        if(modo==1){
          modo=21;
          lcd.setCursor(0,1);
          lcd.print(F("Motor?     "));
          meta=0;
          for(int n=0;n<(sizeof(texto));n++){
            if(texto[n]==' '){              
              meta=0;
            }else{
              meta=meta*10;
              meta+=(texto[n]-0x30);
            }
          }
           
        }else if(modo==21){
          if (motor>0){
            estado_salida[motor-0x31]=1; 
            modo=22;            
          }
        }
       
      }else if(modo==0 || modo==1){
        for (int n='0';n<='9';n++){
          if(tecla==n){
            for(int n1=0;n1<sizeof(texto)-1;n1++){
               texto[n1]=texto[n1+1];               
            }
            texto[sizeof(texto)-1]=tecla;
          }
        }
      }
      break;
  }
  
}

float actualizar_peso(void){
  extern HX711_ADC celdas[3];
  float resultado=0.0;
  for(int n=0;n<3;n++){
    celdas[n].update();
    resultado+=celdas[n].getData();
  }
  return resultado;
}

void imprimir_salidas(void){
  lcd.setCursor(10,1);
      lcd.print("S:");
      for(char n=0;n<4;n++){
        digitalWrite(salida[n],!estado_salida[n]);
        lcd.setCursor(12+n,1);
        lcd.print(estado_salida[n]);
      }
}

void apagar_salidas(void){
  for(int n=0; n<4; n++){
          estado_salida[n]=0;
        }

}

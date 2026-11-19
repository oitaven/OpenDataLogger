// OpenDataLogger 1.0.1 - funcionalidades avancadas.
// Este ficheiro e incluido pelo OpenDataLogger.ino, DEPOIS de jsonText().
// Nao inclui codigo de aquisicao na ISR: calculos e EEPROM sao no loop.

constexpr uint32_t ADV_MAGIC = 0x4F444131UL; // ODA1
constexpr uint16_t ADV_VERSION = 1;
constexpr uint32_t NET_MAGIC = 0x4F444E31UL; // ODN1
constexpr uint8_t MAX_ALARMS = 4;
constexpr uint8_t MAX_VIRTUAL = 4;
constexpr uint8_t MAX_CAL_POINTS = 3;
constexpr uint8_t ALARM_LOG_SIZE = 32;

struct CalibrationPoint { uint16_t raw; float tempC; };
struct CalibrationData { uint8_t count; CalibrationPoint point[MAX_CAL_POINTS]; };
struct VirtualChannel { char name[20]; char color[8]; uint8_t mode; uint8_t a; uint8_t b; };
// Mode: 0=desligado, 1=alto, 2=baixo, 3=ON prolongado,
// 4=OFF prolongado, 5=diferenca A-B elevada,
// 6=AND (canal A digital ON e B analogico acima do limite).
struct AlarmRule { uint8_t mode; uint8_t a; uint8_t b; uint8_t action; float level; uint32_t holdMs; };
struct AdvancedStore {
  uint32_t magic;
  uint16_t version;
  CalibrationData cal[6];
  VirtualChannel virt[MAX_VIRTUAL];
  AlarmRule rules[MAX_ALARMS];
  uint32_t checksum;
};
struct NetSettings {
  uint32_t magic;
  char ssid[2][33];
  char password[2][65];
  char apName[33];
  char apPassword[65];
  uint32_t checksum;
};
struct ShCoeffs { float a,b,c; bool valid; };
struct AlarmState { uint32_t began; bool counting, fired; };
struct AlarmEntry { uint32_t timeMs; float value; uint8_t rule; };
AdvancedStore advancedStore = {};
NetSettings networkSettings = {};
ShCoeffs shCoeffs[6] = {};
AlarmState alarmStates[MAX_ALARMS] = {};
AlarmEntry alarmLog[ALARM_LOG_SIZE] = {};
uint8_t alarmLogUsed = 0;
uint8_t alarmLogHead = 0;
uint32_t alarmTotal = 0;
uint32_t alarmLastEdgeSequence = 0;
uint32_t alarmEdgeGaps = 0;

uint32_t checksumBytes(const void* data, size_t length) {
  const uint8_t* bytes = (const uint8_t*)data;
  uint32_t crc=2166136261UL;
  for(size_t i=0;i<length;++i){crc ^= bytes[i];crc *= 16777619UL;}
  return crc;
}

void advancedDefaults() {
  memset(&advancedStore,0,sizeof(advancedStore));
  advancedStore.magic=ADV_MAGIC;
  advancedStore.version=ADV_VERSION;
  const char* colors[]={"#38bdf8","#f472b6","#fbbf24","#4ade80"};
  for(uint8_t n=0;n<MAX_VIRTUAL;++n){
    snprintf(advancedStore.virt[n].name,20,"Calculado %u",(unsigned)(n+1));
    strcpy(advancedStore.virt[n].color,colors[n]);
    advancedStore.virt[n].a=0;advancedStore.virt[n].b=1;
  }
}

float baselineNTC(uint8_t id,uint16_t raw) {
  if(raw==0||raw>=4095) return NAN;
  const Channel &c=channels[id];
  if(c.rFixed<=0||c.r25<=0||c.beta<=0)return NAN;
  const float r=c.rFixed*raw/(4095.0f-raw);
  return 1.0f/(1.0f/298.15f+logf(r/c.r25)/c.beta)-273.15f;
}

void recomputeSH(uint8_t id) {
  ShCoeffs &sh=shCoeffs[id];sh.valid=false;
  const CalibrationData &cal=advancedStore.cal[id];
  if(cal.count!=3)return;
  double x[3],y[3];
  for(int n=0;n<3;++n){
    const CalibrationPoint &p=cal.point[n];
    if(p.raw==0||p.raw>=4095||p.tempC<=-273.14f)return;
    const double r=(double)channels[id].rFixed*p.raw/(4095.0-p.raw);
    if(r<=0)return;
    x[n]=log(r);y[n]=1.0/(273.15+(double)p.tempC);
  }
  const double x13=x[0]*x[0]*x[0],x23=x[1]*x[1]*x[1],x33=x[2]*x[2]*x[2];
  const double d1=x[1]-x[0], d2=x[2]-x[0];
  const double e1=x23-x13,e2=x33-x13;
  const double v1=y[1]-y[0],v2=y[2]-y[0];
  const double determinant=d1*e2-d2*e1;
  if(fabs(determinant)<1e-10)return;
  const double b=(v1*e2-v2*e1)/determinant;
  const double c=(d1*v2-d2*v1)/determinant;
  const double a=y[0]-b*x[0]-c*x13;
  if(!isfinite(a)||!isfinite(b)||!isfinite(c))return;
  sh.a=(float)a;sh.b=(float)b;sh.c=(float)c;sh.valid=true;
}

float calibratedNTC(uint8_t id,uint16_t raw) {
  if(id>=6)return NAN;
  if(shCoeffs[id].valid && raw>0 && raw<4095){
    const float r=channels[id].rFixed*raw/(4095.0f-raw);
    const float lr=logf(r);
    const ShCoeffs &p=shCoeffs[id];
    const float inv=p.a+p.b*lr+p.c*lr*lr*lr;
    if(inv>0 && isfinite(inv))return 1.0f/inv-273.15f+channels[id].temperatureTrim;
  }
  const float base=baselineNTC(id,raw);
  if(!isfinite(base))return NAN;
  const CalibrationData &cal=advancedStore.cal[id];
  if(cal.count==0)return base+channels[id].temperatureTrim;
  // Com 1-2 pontos, interpolar a correccao da curva beta sem extrapolar.
  float x[3],err[3];
  for(uint8_t i=0;i<cal.count;++i){
    x[i]=baselineNTC(id,cal.point[i].raw);
    if(!isfinite(x[i]))return base+channels[id].temperatureTrim;
    err[i]=cal.point[i].tempC-x[i];
  }
  if(cal.count==1)return base+err[0]+channels[id].temperatureTrim;
  if(x[0]>x[1]) {float t=x[0];x[0]=x[1];x[1]=t;t=err[0];err[0]=err[1];err[1]=t;}
  const float blend=x[1]!=x[0]?constrain((base-x[0])/(x[1]-x[0]),0.0f,1.0f):0.0f;
  return base+err[0]+(err[1]-err[0])*blend+channels[id].temperatureTrim;
}

void networkDefaults(){
  memset(&networkSettings,0,sizeof(networkSettings));
  networkSettings.magic=NET_MAGIC;
  for(uint8_t i=0;i<2;++i){
    if(wifiNames[i])strncpy(networkSettings.ssid[i],wifiNames[i],32);
    if(wifiPass[i])strncpy(networkSettings.password[i],wifiPass[i],64);
  }
  strncpy(networkSettings.apName,AP_SSID,32);
  strncpy(networkSettings.apPassword,AP_PASS,64);
}

void loadAdvancedSettings(){
  EEPROM.get((int)sizeof(StoredSettings),advancedStore);
  const uint32_t chk=checksumBytes(&advancedStore,offsetof(AdvancedStore,checksum));
  if(advancedStore.magic!=ADV_MAGIC||advancedStore.version!=ADV_VERSION||advancedStore.checksum!=chk)advancedDefaults();
  for(uint8_t i=0;i<6;++i){
    if(advancedStore.cal[i].count>3)advancedStore.cal[i].count=0;
    recomputeSH(i);
  }
  EEPROM.get((int)(sizeof(StoredSettings)+sizeof(AdvancedStore)),networkSettings);
  const uint32_t netChk=checksumBytes(&networkSettings,offsetof(NetSettings,checksum));
  if(networkSettings.magic!=NET_MAGIC||networkSettings.checksum!=netChk||
     networkSettings.apPassword[64]!='\0'||strlen(networkSettings.apPassword)<8||
     networkSettings.apName[32]!='\0'||
     networkSettings.ssid[0][32]!='\0'||networkSettings.ssid[1][32]!='\0'||
     networkSettings.password[0][64]!='\0'||networkSettings.password[1][64]!='\0') networkDefaults();
}

void saveAdvancedSettings(){
  advancedStore.magic=ADV_MAGIC;advancedStore.version=ADV_VERSION;
  advancedStore.checksum=checksumBytes(&advancedStore,offsetof(AdvancedStore,checksum));
  EEPROM.put((int)sizeof(StoredSettings),advancedStore);
}
void saveNetworkSettings(){
  networkSettings.magic=NET_MAGIC;
  networkSettings.checksum=checksumBytes(&networkSettings,offsetof(NetSettings,checksum));
  EEPROM.put((int)(sizeof(StoredSettings)+sizeof(AdvancedStore)),networkSettings);
}

void resetAlarms(){
  memset(alarmStates,0,sizeof(alarmStates));
  alarmLastEdgeSequence=digitalEdgeSerial;
}
void raiseAlarm(uint8_t index,uint32_t at,float value){
  if(index>=MAX_ALARMS)return;
  AlarmEntry &entry=alarmLog[alarmLogHead];
  entry.timeMs=at;entry.value=value;entry.rule=index;
  alarmLogHead=(uint8_t)((alarmLogHead+1)%ALARM_LOG_SIZE);
  if(alarmLogUsed<ALARM_LOG_SIZE)++alarmLogUsed;
  ++alarmTotal;
  if(advancedStore.rules[index].action==1&&triggerState==1)activateTrigger(at);
}

// Consome flancos previamente recolhidos pela ISR, sem executar regras
// na interrupcao. Impulsos ON de 250 ms sao avaliados mesmo com ADC a 10 s.
void processDigitalAlarmEvents(){
  for(uint8_t n=0;n<32;++n){
    DigitalEdge event={};bool found=false;
    noInterrupts();
    const uint8_t count=digitalEdgeUsed;
    if(count){
      const uint8_t first=(uint8_t)((digitalEdgeHead+DIGITAL_EVENT_CAPACITY-count)%DIGITAL_EVENT_CAPACITY);
      const uint32_t oldest=digitalEdges[first].sequence;
      uint32_t wanted=alarmLastEdgeSequence+1;
      if(wanted<oldest){alarmEdgeGaps+=(oldest-wanted);wanted=oldest;}
      const uint32_t delta=wanted-oldest;
      if(delta<count){event=digitalEdges[(first+delta)%DIGITAL_EVENT_CAPACITY];found=true;}
    }
    interrupts();
    if(!found)break;
    alarmLastEdgeSequence=event.sequence;
    for(uint8_t i=0;i<MAX_ALARMS;++i){
      const AlarmRule &r=advancedStore.rules[i];
      if((r.mode!=3 && r.mode!=4)||r.a!=event.channel)continue;
      AlarmState &st=alarmStates[i];
      const bool desired=r.mode==3;
      if((event.level!=0)==desired){
        st.counting=true;st.fired=false;st.began=event.timeMs;
        if(r.holdMs==0){st.fired=true;raiseAlarm(i,event.timeMs,event.level?1.0f:0.0f);}
      }else if(st.counting){
        if(!st.fired && (uint32_t)(event.timeMs-st.began)>=r.holdMs){
          st.fired=true;raiseAlarm(i,st.began+r.holdMs,desired?1.0f:0.0f);
        }
        st.counting=false;st.fired=false;
      }
    }
  }
}

void evaluateAlarms(uint32_t now){
  for(uint8_t i=0;i<MAX_ALARMS;++i){
    const AlarmRule &r=advancedStore.rules[i];
    if(r.mode==0||r.a>=CHANNELS||r.b>=CHANNELS)continue;
    if(channels[r.a].type==OFF)continue;
    if((r.mode==5||r.mode==6) && channels[r.b].type==OFF)continue;
    if((r.mode==3||r.mode==4||r.mode==6) &&
       (r.a<6 || channels[r.a].type!=DIGITAL))continue;
    const float a=sampleValue(r.a,latest[r.a]);
    const float b=sampleValue(r.b,latest[r.b]);
    bool condition=false;float actual=a;
    switch(r.mode){
      case 1:condition=a>r.level;break;
      case 2:condition=a<r.level;break;
      case 3:condition=r.a>=6 && digitalPrevious[r.a-6]!=0;actual=1;break;
      case 4:condition=r.a>=6 && digitalPrevious[r.a-6]==0;actual=0;break;
      case 5:actual=a-b;condition=actual>r.level;break;
      case 6:actual=b;condition=r.a>=6&&digitalPrevious[r.a-6]!=0&&b>r.level;break;
    }
    if(!isfinite(actual))condition=false;
    AlarmState &st=alarmStates[i];
    if(!condition){st.counting=false;st.fired=false;continue;}
    if(!st.counting){st.counting=true;st.began=now;}
    if(!st.fired&&(uint32_t)(now-st.began)>=r.holdMs){
      st.fired=true;
      raiseAlarm(i,now,actual);
    }
  }
}

void sendAdvancedStatus(Print &out){
  out.print(F(",\"virtuals\":["));
  for(uint8_t i=0;i<MAX_VIRTUAL;++i){
    if(i)out.print(',');
    const VirtualChannel &v=advancedStore.virt[i];
    out.print(F("{\"id\":"));out.print(8+i);
    out.print(F(",\"name\":"));jsonText(out,v.name);
    out.print(F(",\"color\":"));jsonText(out,v.color);
    out.print(F(",\"mode\":"));out.print(v.mode);
    out.print(F(",\"a\":"));out.print(v.a);
    out.print(F(",\"b\":"));out.print(v.b);
    out.print('}');
  }
  out.print(F("],\"alarms\":["));
  for(uint8_t i=0;i<MAX_ALARMS;++i){
    if(i)out.print(',');
    const AlarmRule &r=advancedStore.rules[i];
    out.print(F("{\"id\":"));out.print(i);
    out.print(F(",\"mode\":"));out.print(r.mode);
    out.print(F(",\"a\":"));out.print(r.a);
    out.print(F(",\"b\":"));out.print(r.b);
    out.print(F(",\"action\":"));out.print(r.action);
    out.print(F(",\"level\":"));out.print(r.level,5);
    out.print(F(",\"holdMs\":"));out.print(r.holdMs);
    out.print('}');
  }
  out.print(F("],\"calibration\":["));
  for(uint8_t i=0;i<6;++i){
    if(i)out.print(',');
    out.print(F("{\"channel\":"));out.print(i);
    out.print(F(",\"points\":["));
    for(uint8_t j=0;j<advancedStore.cal[i].count;++j){
      if(j)out.print(',');
      out.print('[');out.print(advancedStore.cal[i].point[j].raw);
      out.print(',');out.print(advancedStore.cal[i].point[j].tempC,3);out.print(']');
    }
    out.print(F("],\"sh\":"));
    if(shCoeffs[i].valid){
      out.print('[');out.print(shCoeffs[i].a,9);out.print(',');
      out.print(shCoeffs[i].b,9);out.print(',');out.print(shCoeffs[i].c,12);out.print(']');
    } else out.print(F("null"));
    out.print('}');
  }
  out.print(F("],\"networkSetup\":{\"ssid0\":"));jsonText(out,networkSettings.ssid[0]);
  out.print(F(",\"ssid1\":"));jsonText(out,networkSettings.ssid[1]);
  out.print(F(",\"apName\":"));jsonText(out,networkSettings.apName);
  out.print(F(",\"accessPoint\":"));out.print(usingAccessPoint?1:0);
  out.print(F("},\"alarmCount\":"));out.print(alarmTotal);
}

void updateVirtual(const String &url){
  const int idx=param(url,"index").toInt();
  if(idx<0||idx>=MAX_VIRTUAL)return;
  VirtualChannel &v=advancedStore.virt[idx];
  const int mode=param(url,"mode").toInt(),a=param(url,"a").toInt(),b=param(url,"b").toInt();
  if(mode<0||mode>5||a<0||a>=CHANNELS||b<0||b>=CHANNELS)return;
  v.mode=mode;v.a=a;v.b=b;
  String name=param(url,"name"),color=param(url,"color");
  if(name.length()&&name.length()<20)name.toCharArray(v.name,sizeof(v.name));
  if(color.length()==7&&color[0]=='#')color.toCharArray(v.color,sizeof(v.color));
}

void updateAlarm(const String &url){
  const int idx=param(url,"index").toInt();
  if(idx<0||idx>=MAX_ALARMS)return;
  const int mode=param(url,"mode").toInt(),a=param(url,"a").toInt(),b=param(url,"b").toInt(),action=param(url,"action").toInt();
  const float level=param(url,"level").toFloat();
  const uint32_t hold=(uint32_t)strtoul(param(url,"holdMs").c_str(),nullptr,10);
  if(mode<0||mode>6||a<0||a>=CHANNELS||b<0||b>=CHANNELS||action<0||action>1||
     !isfinite(level)||fabsf(level)>100000||hold>86400000UL)return;
  AlarmRule &r=advancedStore.rules[idx];
  r.mode=mode;r.a=a;r.b=b;r.action=action;r.level=level;r.holdMs=hold;
  alarmStates[idx]={};
}

void updateCalibrationPoint(const String &url){
  const int id=param(url,"channel").toInt();
  if(id<0||id>=6||channels[id].type!=NTC)return;
  CalibrationData &cal=advancedStore.cal[id];
  if(param(url,"action")=="clear"){
    cal.count=0;recomputeSH(id);return;
  }
  const float reference=param(url,"temp").toFloat();
  const String rawParam=param(url,"raw");
  const uint16_t raw=rawParam.length()?(uint16_t)rawParam.toInt():latest[id];
  if(cal.count>=3||raw<1||raw>4094||!isfinite(reference)||reference<-100||reference>150)return;
  // Evita pontos duplicados, que tornam a calibracao SH singular.
  for(uint8_t j=0;j<cal.count;++j)if(abs((int)raw-(int)cal.point[j].raw)<5)return;
  cal.point[cal.count].raw=raw;
  cal.point[cal.count].tempC=reference;
  ++cal.count;recomputeSH(id);
}

void sendAlarmLog(WiFiClient &c){
  BufferedHttpPrint out(c);headers(out,"application/json");
  out.print(F("{\"total\":"));out.print(alarmTotal);
  out.print(F(",\"events\":["));
  for(uint8_t i=0;i<alarmLogUsed;++i){
    const uint8_t slot=(uint8_t)((alarmLogHead+ALARM_LOG_SIZE-alarmLogUsed+i)%ALARM_LOG_SIZE);
    const AlarmEntry &e=alarmLog[slot];
    if(i)out.print(',');
    out.print(F("{\"t\":"));out.print(e.timeMs);
    out.print(F(",\"rule\":"));out.print(e.rule);
    out.print(F(",\"value\":"));out.print(e.value,4);out.print('}');
  }
  out.println(F("]}"));out.finish();
}

void sendHttpError(WiFiClient &c,const char* code,const char* msg){
  c.print(F("HTTP/1.1 "));c.println(code);
  c.println(F("Content-Type: application/json"));
  c.println(F("Cache-Control: no-store"));
  c.println(F("Connection: close"));c.println();
  c.print(F("{\"ok\":false,\"error\":"));jsonText(c,msg);c.println('}');
}

bool applyNetworkPost(const String &body){
  const String supplied=param(body,"admin");
  if(strlen(ODL_WEB_ADMIN_KEY)<12 || strcmp(ODL_WEB_ADMIN_KEY,"ALTERAR_CODIGO_ADMIN_FORTE")==0 ||
     supplied!=ODL_WEB_ADMIN_KEY)return false;
  NetSettings newer=networkSettings;
  for(uint8_t i=0;i<2;++i){
    const String ssid=param(body,i==0?"ssid0":"ssid1");
    const String pass=param(body,i==0?"pass0":"pass1");
    if(ssid.length()>32||pass.length()>64)return false;
    ssid.toCharArray(newer.ssid[i],33);
    if(pass.length())pass.toCharArray(newer.password[i],65);
    if(!ssid.length())newer.password[i][0]=0;
  }
  const String apName=param(body,"apName"),apPassword=param(body,"apPassword");
  if(apName.length()<1||apName.length()>32||apPassword.length()>64)return false;
  apName.toCharArray(newer.apName,33);
  if(apPassword.length()){
    if(apPassword.length()<8)return false;
    apPassword.toCharArray(newer.apPassword,65);
  }
  if(strlen(newer.apPassword)<8)return false;
  networkSettings=newer;
  saveNetworkSettings();
  return true;
}

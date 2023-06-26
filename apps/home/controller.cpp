#include "controller.h"
#include "app.h"
#include <apps/home/apps_layout.h>
#include "../apps_container.h"
#include "../global_preferences.h"
#include "../exam_mode_configuration.h"

extern "C" {
#include <assert.h>
}

#ifdef NSPIRE_NEWLIB
const char * storage_name="/documents/nwspire.nws.tns";
#endif
#ifdef __EMSCRIPTEN__
const char * storage_name="/data/nwsweb.nws";
int copy_host_to_nw(const char * hostname,const char * nwname,int autoexec);
extern "C" void copy_to_fs(const char * filename,const char * data,size_t length);
extern "C" int save_state(const char * fname);
// from JS run cp=CalcModule.cwrap('copy_to_fs', 'null', ['string','string','number'])
// then e.g. cp("/data/aa.py","123",3)
#else
void console_log(const char *){}
#endif
#ifdef _FXCG
const char * storage_name="nwcasio.nws";
#endif

const char * calc_storage_name="nwcalc.txt";
void * storage_address(); // ion/src/simulator/shared/platform_info.cpp
const char * retrieve_calc_history(); 
#ifdef XCAS
extern "C" void restore_calc_history();
extern "C" int save_calc_history();
void display_host_help();
void display_giac_license(bool chkcas);
int load_state(const char * fname);
#endif

// Additional code by B. Parisse for host file system support and persistence
// on Casio Graph 90/FXCG50 and TI Nspire
void erase_record(const char * name){
  unsigned char * ptr=(unsigned char *)storage_address();
  for (ptr+=4;;){ // skip header
    size_t L=ptr[1]*256+ptr[0]; 
    if (L==0) return;
    if (strcmp((const char *)ptr+2,name)==0){
      unsigned char * newptr=ptr;
      int S=0,erased=L;
      ptr+=L;
      for (;;){
	L=ptr[1]*256+ptr[0];
	if (L==0){
	  for (int i=0;i<erased;++i)
	    newptr[S+i]=0;
	  break;
	}
	// if (ptr>newptr+S)
	memmove(newptr+S,ptr,L);
	S+=L;
	ptr+=L; 
      }
      return;
    }
    ptr+=L;
  }
}


// record filtering on read
void filter(unsigned char * ptr){
  unsigned char * newptr=ptr;
  int S; ptr+=4;
  for (S=4;;){
    size_t L=ptr[1]*256+ptr[0];
    if (L==0) break;
    int l=strlen((const char *)ptr+2);
    // filter py and txt records
    if (l>3 &&
	(strncmp((const char *)ptr+2+l-3,".py",3)==0 ||
	 strncmp((const char *)ptr+2+l-4,".txt",4)==0)){
      // if (ptr>newptr+S)
      memmove(newptr+S,ptr,L);
      S+=L;
    }
#if 0 // def STRING_STORAGE
    if (l>5 && strncmp((const char *)ptr+2+l-5,".func",5)==0){
      int shift=l+4+13;
      Ion::Storage::Record * record=(Ion::Storage::Record *)malloc(1024);
      memcpy(record,ptr,L);
      //ExpressionModelHandle
      Poincare::Expression e=Poincare::Expression::Parse((const char *)ptr+shift,NULL);
      //ExpressionModel::setContent(Ion::Storage::Record * record, const char * c, Context * context, CodePoint symbol);
      Shared::ExpressionModel md;
      Ion::Storage::Record::ErrorStatus err=md.setExpressionContent(record, e);
      if (1){
	// if (ptr>newptr+S)
	int newL=record->value().size;
	memmove(newptr+S,record,newL);
	S+=newL;
      }
      free(record);
    }
#endif
    ptr+=L; 
  }
}


#ifdef NSPIRE_NEWLIB
#include "../../ion/src/simulator/nspire/k_csdk.h"
#endif // NSPIRE_NEWLIB

#ifdef _FXCG
#include "../../ion/src/simulator/fxcg/platform.h"
#include "../../ion/src/simulator/fxcg/menuHandler.h"
#include <gint/bfile.h>
#include <gint/display-cg.h>
#include <gint/gint.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#define KEY_CTRL_OK KEY_EXE
inline int do_getkey(){
  return getkey().key;
}
#endif

#ifdef __EMSCRIPTEN__
#include "../../ion/src/simulator/web/web.h"
void writepc(const unsigned char * ptr,size_t L,const char * filename){
  EM_ASM({
      let ptr=$0;
      let len=$1;
      let buf=HEAPU8.subarray(ptr, ptr+len);
      //console.log(buf);
      const blob = new Blob([buf], {type: 'application/octet-binary'});
      //console.log(blob);
      const url = window.URL.createObjectURL(blob);
      var link = document.createElement('a');
      var filename=UTF8ToString($2);
      console.log(filename);
      link.download = filename;
      link.href=url;
      //console.log(link);
      link.click();
      //window.URL.revokeObjectURL(url);
    },ptr,L,filename);
}

void copy_to_fs(const char * filename,const char * data,size_t length){
  const char *nwname=filename+strlen(filename);
  const char * ext=0;
  for (;nwname>filename;--nwname){
    if (*nwname=='/'){
      ++nwname;
      break;
    }
    if (*nwname=='.')
      ext=nwname;
  }
  std::string fname(filename);
  if (nwname==filename)
    fname="/data/"+fname;
  FILE * f=fopen(fname.c_str(),"wb");
  if (!f){
    EM_ASM({
	alert('Unable to open '+UTF8ToString($0));
      },filename);
    return;
  }
  fwrite(data,1,length,f);
  fclose(f);
  if (strncmp(ext,".nws",4)==0){
    int i=EM_ASM_INT({
	return confirm('Ecraser etat courant par la sauvegarde?');
      });
    if (i){
      load_state(fname.c_str());
      restore_calc_history();
    }
  }
  else {
    int res=copy_host_to_nw(fname.c_str(),nwname,1);
    if (res==-2){
      EM_ASM({
	  alert('Not enough space to store in Upsilon');
	});
    }
  }
}
#endif


#ifdef XCAS
#include <ion/storage.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <vector>
#include <string>

#include <ion.h>
#include <ion/events.h>
#include "../calculation/calculation_store.h"
const int storage_length=60000; // 60000 in Upsilon, 32768 in Epsilon
// k_storageSize = 60000; in ion/include/ion/internal_storage.h
extern void * last_calculation_history;

#ifdef _FXCG
int load_state(const char * fname){
  FILE * f=fopen(fname,"rb");
  if (f){
    unsigned char * ptr=(unsigned char *)storage_address();
    ptr[3]=fgetc(f);
    ptr[2]=fgetc(f);
    ptr[1]=fgetc(f);
    ptr[0]=fgetc(f);
    fread(ptr+4,1,storage_length-4,f);
    fclose(f);
    if (strcmp(storage_name,fname)) // keep py and calc history
      filter(ptr);
    return 1;
  }
  return 0;
}
#else
int load_state(const char * fname){
  console_log(fname);
  FILE * f=fopen(fname,"rb");
  if (f){
    console_log("found");
    unsigned char * ptr=(unsigned char *)storage_address();
    fread(ptr,1,storage_length,f);
    fclose(f);
    if (strcmp(storage_name,fname)) // keep py and calc history
      filter(ptr);
    return 1;
  }
  return 0;
}
#endif // else FXCG

#ifdef _FXCG
static void convert(const char * fname,unsigned short * pFile){
  for ( ;*fname;++fname,++pFile)
    *pFile=*fname;
  *pFile=0;
}

int save_state(const char * fname){
  //return 0;
  if (Ion::Storage::sharedStorage()->numberOfRecords()){
#if 0
    const char prefix[]="\\\\fls0\\";
    char buf[512];
    strcpy(buf,prefix);
    strcat(buf,fname);
    unsigned short pFile[512];
    convert(buf,pFile);
    int hf = BFile_Open(pFile, BFile_WriteOnly); // Get handle
    // cout << hf << endl << "f:" << filename << endl; Console_Disp();
    if (hf<0){
      int l=storage_length;
      BFile_Create(pFile,0,&l);
      hf = BFile_Open(pFile, BFile_WriteOnly);
    }
    if (hf < 0)
      return 0;
    int l=BFile_Write(hf,storage_address(),storage_length);
    BFile_Close(hf);
    if (l==storage_length)
      return 1;
    return -1;
#else
    const unsigned char * ptr=(const unsigned char *)storage_address();
    // find store size
    int S=4; 
    for (ptr+=4;;){ // skip header
      size_t L=ptr[1]*256+ptr[0];  
      ptr+=L;
      S+=L;
      if (L==0) break;
    }
    S = ((S+1023)/1024)*1024;
    FILE * f=fopen(fname,"wb");
    if (f){
      ptr=(const unsigned char *) storage_address();
      fputc(ptr[3],f);
      fputc(ptr[2],f);
      fputc(ptr[1],f);
      fputc(ptr[0],f);
      //fwrite(ptr+4,1,S-4,f);
      fwrite(ptr+4,S-4,1,f);
      fclose(f);
      return S;
    }
    return 0;
#endif
  }
  return 2;
}

#else // _FXCG

int save_state(const char * fname){
  if (1 || Ion::Storage::sharedStorage()->numberOfRecords()){
    const unsigned char * ptr=(const unsigned char *)storage_address();
    // find store size
    int S=4; 
    for (ptr+=4;;){ // skip header
      size_t L=ptr[1]*256+ptr[0];  
      ptr += L;
      S += L;
      if (L==0){
	S += 2; // keep 2 zeros at end of file
	break;
      }
    }
    S = ((S+1023)/1024)*1024;
    if (strcmp(storage_name,fname)){
      // keep only python scripts and txt records
      unsigned char * newptr=(unsigned char *) malloc(S);
      bzero(newptr,S);
      ptr=(const unsigned char *) storage_address();
      memcpy(newptr,ptr,4); ptr+=4;
      for (S=4;;){
	size_t L=ptr[1]*256+ptr[0];
	if (L==0) break;
	int l=strlen((const char *)ptr+2);
	if (l>3 &&
	    (strncmp((const char *)ptr+2+l-3,".py",3)==0 ||
	     strncmp((const char *)ptr+2+l-4,".txt",4)==0)
	    ){
	  memcpy(newptr+S,ptr,L);
	  S+=L;
	}
	ptr+=L; 
      }
      S = ((S+1023)/1024)*1024;
      FILE * f;
      f=fopen(fname,"wb");
      if (f){
	fwrite(newptr,S,1,f);
	//fwrite(ptr+4,1,S-4,f);
	fclose(f);
	free(newptr);
#ifdef __EMSCRIPTEN__
	if (strcmp(fname,storage_name)==0)
	  writepc(ptr,S,"nwstore.nws");
	else
	  writepc(ptr,S,fname);
	EM_ASM({
	    FS.syncfs(function (err) {
		console.log('syncfs write error=',err);
		if (err) alert('Error saving state');
	      });
	  });
#endif
	return S;
      }
      free(newptr);
      return 0;
    }
    else { // no filter
      ptr=(const unsigned char *)storage_address();
      FILE * f;
      f=fopen(fname,"wb");
      if (f){
	fwrite(ptr,S,1,f);
	//fwrite(ptr+4,1,S-4,f);
	fclose(f);
#ifdef __EMSCRIPTEN__
	if (strcmp(fname,storage_name)==0)
	  writepc(ptr,S,"nwstore.nws");
	else
	  writepc(ptr,S,fname);
	EM_ASM({
	    FS.syncfs(function (err) {
		console.log('syncfs write error=',err);
		if (err) alert('Error saving state');
	      });
	  });
#endif
	return S;
      }
      return 0;
    } // end else filtering
  }
  return 2;
}
#endif // else _FXCG

extern "C" void load_nwstate();

void load_nwstate(){
  load_state(storage_name);
}

#endif // XCAS

const char * calc_history(){
  unsigned char * ptr=(unsigned char *)storage_address();
  for (ptr+=4;;){ // skip header
    size_t L=ptr[1]*256+ptr[0]; 
    if (L==0) return 0;
    if (strcmp((const char *)ptr+2,calc_storage_name)==0){
      const char * buf=(const char *)ptr+2+strlen(calc_storage_name)+1;
      console_log("calc_history");
      console_log(buf);
      return buf;
    }
    ptr += L;
  }
  return 0;
}


const char * retrieve_calc_history(){
#ifdef XCAS
  static bool firstrun=true;
  if (firstrun){
    firstrun=false;
#ifdef __EMSCRIPTEN__ // loading state must be done after syncfs
    // init filesystem
    EM_ASM({
	FS.mkdir('/data');
	FS.mount(IDBFS, { }, '/data');
	if (confirm('Restore previous state?')){
	  FS.syncfs(true,function (err) {
	      console.log('syncfs read, error=',err);
	      ccall('load_nwstate','v');
	      ccall('restore_calc_history','v');
	    });
	}
      });
    return 0;
#else // emscripten
#ifdef _FXCG
    int l=gint_world_switch(GINT_CALL(load_state,storage_name));
#else
    int l=load_state(storage_name);
#endif // _FXCG
    if (l==0){
      display_host_help();
      // ((App*)m_app)->redraw();      
    }
#endif // emscripten
  }
#endif // xcas
  return calc_history();
}

#ifdef XCAS
std::string retrieve_functions();
std::string retrieve_functions(){
  const unsigned char * ptr=(const unsigned char *)storage_address();
  std::string s;
  for (ptr+=4;;){ // skip header
    size_t L=ptr[1]*256+ptr[0]; 
    if (L==0) return s; // not found
    char * name=(char *)ptr+2;
    int l=strlen(name);
#if 0
    // sequence support would require a global command nwseq where
    // all sequences are given as arguments, because some recurrence
    // relations may mix different sequences
    const char extseq[]=".seq";
    if (l>strlen(extseq) && strncmp(name+l-strlen(extseq),extseq,strlen(extseq))==0){
      // From start record,
      // u.seq, 4 bytes, 0 or 1 or 2 (type of sequence), 5 bytes, "expression",
      // u0 (type 1) or u0 and u1 (type 2) position shift?
      // u(n+1) in terms of u(n) or u(n+2) of u(n+1) and u(n)
    }
#endif
    const char extfunc[]=".func";
    if (l<=int(strlen(extfunc)) || strncmp(name+l-strlen(extfunc),extfunc,strlen(extfunc))!=0){
      ptr += L;
      continue;
    }
    char c=name[l-strlen(extfunc)];
    name[l-strlen(extfunc)]=0;
    s += name;
    name[l-strlen(extfunc)]=c;
    s +="(x):=";
    for (const char * ptr=name+l+15;*ptr && *ptr!='"';++ptr){
      if (*ptr==(char)0xc3 && ptr[1]==(char)0x97){
	s += '*';
	++ptr;
	continue;
      }
      if (*ptr==(char)0xe2 && ptr[1]==(char)0x88 && ptr[2]==(char)0x9e){
	s += "inf";
	ptr+=2;
	continue;
      }
      s += *ptr;
    }
    s += ";";
#if 0
    dclear(C_WHITE);
    dtext(1,2, C_BLACK,"Functions");
    dtext(1,20,C_BLACK,s.c_str());
    dupdate(); getkey();
#endif	
    ptr+=L;
  }
}

int save_calc_history(){
  if (!last_calculation_history)
    return false;
  erase_record(calc_storage_name);
  std::string s;
  Calculation::CalculationStore * store=(Calculation::CalculationStore *) last_calculation_history;
  int N=store->numberOfCalculations();
  for (int i=N-1;i>=0;--i){
    s += store->calculationAtIndex(i)->inputText();
    s += '\n';
  }
  if (s.empty())
    return false;
  Ion::Storage::Record::ErrorStatus  res= Ion::Storage::sharedStorage()->createRecordWithFullName(calc_storage_name,&s[0],s.size());
  if (res==Ion::Storage::Record::ErrorStatus::NotEnoughSpaceAvailable)
    return false;
  return true;
}

void confirm_load_state(const char * buf){
  bool fr=GlobalPreferences::sharedGlobalPreferences()->language()==I18n::Language(1);
  dclear(C_WHITE);
  dtext(1,1, C_BLACK, fr?"Chargement depuis fichier":"Loading from state file");
  dtext(1,17,C_BLACK,buf);
  dtext(1,33,C_BLACK,fr?"Le contexte actuel sera perdu!":"Current context will be lost!");
  dtext(1,49,C_BLACK,fr?"Tapez EXE ou enter pour confirmer":"Press EXE to confirm");
  dupdate();
  int k=do_getkey();
  if (k==KEY_EXE || k==KEY_CTRL_OK){
#ifdef _FXCG
    int l=gint_world_switch(GINT_CALL(load_state,buf));
#else
    int l=load_state(buf);
#endif
    char buf2[]="0";
    buf2[0] += l;
    if (l==0)
      dtext(1,65,C_BLACK,fr?"Erreur en lisant l'etat":"Error reading state");
    if (l==1)
      dtext(1,65,C_BLACK,fr?"Lecture de l'etat reussie":"Success reading state");
    dtext(1,81,C_BLACK,buf2);
    dtext(1,97,C_BLACK,fr?"Tapez une touche":"Press any key");
    dupdate();
    do_getkey();
  }
}  

struct file {
  std::string s;
  int length;
  bool isdir;
};

void host_scripts(std::vector<file> & v,const char * dirname,const char * extension){
#ifdef __EMSCRIPTEN__
  console_log(dirname);
#endif
  v.clear();
  file f={".._parent_dir",0,true};
  if (strlen(dirname)>1)
    v.push_back(f);
  DIR *dp;
  struct dirent *ep;  
  dp = opendir (dirname);
  int l=extension?strlen(extension):0;
  if (dp != NULL){
    int t;
    while ( (ep = readdir (dp)) ){
      if (strlen(ep->d_name)>=1 && ep->d_name[0]=='.')
	continue;
      f.s=ep->d_name;
      if (f.s=="@MainMem")
	continue;
#ifdef NSPIRE_NEWLIB
      DIR * chk=opendir((dirname+f.s).c_str());
      f.isdir=true;
      if (chk)
	closedir(chk);
      else
	f.isdir=false; 
#else
      f.isdir=ep->d_type==DT_DIR;
#endif
#if 1
      if (f.isdir)
	f.length=0;
      else {
	struct stat st;
	stat((dirname+f.s).c_str(), &st);
	f.length = st.st_size;
	if (f.length>=32768)
	  continue;
      }
#else
      f.length=f.isdir?0:-1;
#endif
      if (f.isdir || !extension)
	v.push_back(f);
      else {
	t=strlen(ep->d_name);
	if (t>l && strncmp(ep->d_name+t-l,extension,l)==0)
	  v.push_back(f);
      }
    }
    closedir (dp);
  }
}

void nw_scripts(std::vector<file> & v,const char * extension){
  v.clear();
#if 0
 int n=Ion::Storage::sharedStorage()->numberOfRecords();
 for (int i=0;i<n;++i){
   v.push_back(Ion::Storage::sharedStorage()->recordAtIndex(i).fullName());
 }
#else
  const unsigned char * ptr=(const unsigned char *)storage_address();
  int l=extension?strlen(extension):0;
  for (ptr+=4;;){ // skip header
    size_t L=ptr[1]*256+ptr[0]; 
    ptr+=2; 
    if (L==0) break;
    L-=2;
    file f={(const char *)ptr,(int)L,false};
    if (!extension)
      v.push_back(f);
    else {
      int namesize=strlen((const char *)ptr);
      if (namesize>l && strncmp((const char *)ptr+namesize-l,extension,l)==0)
	v.push_back(f);
    }
    ptr+=L;
  }
#endif
}

int copy_nw_to_host(const char * nwname,const char * hostname){
#ifdef NSPIRE_NEWLIB
  int s=strlen(hostname);
  if (s<4 || strncmp(hostname+s-4,".tns",4)){
    std::string S(hostname);
    S+=".tns";
    return copy_nw_to_host(nwname,S.c_str());
  }
#endif
  const unsigned char * ptr=(const unsigned char *)storage_address();
  for (ptr+=4;;){ // skip header
    size_t L=ptr[1]*256+ptr[0]; 
    if (L==0) return 3; // not found
    //dclear(C_WHITE);
    //dtext(1,1,C_BLACK,ptr+2);
    //dtext(1,17,C_BLACK,nwname);
    //dupdate();
    //getkey();
    if (strcmp((const char *)ptr+2,nwname)){
      ptr += L;
      continue;
    }
    ptr+=2; 
    L-=2;
    int l=strlen((const char *)ptr);
    ptr += l+2;
    L -= (l+2);
    int L1=L;
    L = 2*((L+1)/2);
    FILE * f=fopen(hostname,"wb");
    if (!f)
      return 2;
    fwrite(ptr,1,L,f);
    fclose(f);
#ifdef __EMSCRIPTEN__
    writepc(ptr,L,nwname);
#endif
    return 0;
  }
  return 1;
}

int copy_host_to_nw(const char * hostname,const char * nwname,int autoexec){
  console_log("copy_host_to_nw");
  console_log(hostname);
  console_log(nwname);
  FILE * f=fopen(hostname,"rb");
  if (!f)
    return -1;
  std::vector<unsigned char> v(1,autoexec?1:0);
  for (;;){
    unsigned char c=fgetc(f);
    if (feof(f)){
      if (c>=' ' && c<=0x7e)
	v.push_back(c);
      break;
    }
    if (c==0xa && !v.empty() && v.back()==0xd)
      v.back()=0xa;
    else
      v.push_back(c);
  }
  if (!v.empty() && v.back()!=0xa)
    v.push_back(0xa);
  v.push_back(0);
  fclose(f);
  if (Ion::Storage::sharedStorage()->hasRecord(Ion::Storage::sharedStorage()->recordNamed(nwname)))
    Ion::Storage::sharedStorage()-> destroyRecord(Ion::Storage::sharedStorage()->recordNamed(nwname));
  Ion::Storage::Record::ErrorStatus  res= Ion::Storage::sharedStorage()->createRecordWithFullName(nwname,&v.front(),v.size());
  if (res==Ion::Storage::Record::ErrorStatus::NotEnoughSpaceAvailable)
    return -2;
  return 0;
}

bool filesort(const file & a,const file & b){
  if (a.isdir!=b.isdir)
    return a.isdir;
  return a.s<b.s;
}

void display_host_help();
void display_host_help(){
  dclear(C_WHITE);
  if (GlobalPreferences::sharedGlobalPreferences()->language()==I18n::Language(1)){
#ifdef _FXCG
    dtext(1,2, C_BLUE,"Clavier: OPTN=Toolbox, EXIT=Back, MENU=Home");
    dtext(1,20,C_BLUE, "Depuis home, taper OPTN affiche cette aide,");
    dtext(1,38,C_BLUE, "  taper MENU ou DEL quitte Upsilon.");
    dtext(1,56,C_BLACK,"Sauvegarde etat: depuis Home taper la touche ->");
    dtext(1,74,C_BLACK,"Pour echanger des fichiers ou sauvegardes:");
    dtext(1,92,C_BLACK,"  depuis home, taper VARS");
    dtext(1,110,C_BLACK,"Pour effacer l'etat d'Upsilon: depuis le MENU Casio");
    dtext(1,128,C_BLACK,"choisir Memoire, taper F2, effacer nwstore.nws");
#else
#define C_BLUE 31
    dtext(1,2, C_BLUE,"Clavier: menu=Toolbox, esc=Back, doc=Home");
    dtext(1,20,C_BLUE ,"Depuis home, taper ON ou doc quitte Upsilon");
    dtext(1,38,C_BLUE ,"  Ctrl K eteint l'ecran, ON rallume.");
    dtext(1,56,C_BLACK,"Sauvegarde etat: depuis Home taper ctrl VAR");
    dtext(1,74,C_BLACK,"Pour echanger des fichiers ou sauvegardes:");
    dtext(1,92,C_BLACK,"  depuis home, taper VARS");
    dtext(1,110,C_BLACK,"Effacer l'etat d'Upsilon: depuis l'accueil TI");
    dtext(1,128,C_BLACK,"taper 2, effacer nwstore.nws dans My documents");
    os_wait_1ms(100);
#endif
    dtext(1,150,C_BLACK,"Upsilon est issu d'Epsilon (c) Numworks");
    dtext(1,168,C_BLACK,"https://getupsilon.web.app/");
    dtext(1,186,C_BLACK,"Persistance et echanges fichiers (c) B. Parisse");
    dtext(1,204,C_BLACK,"License creativecommons.org CC BY-NC-SA 4.0");
  }
  else {
#ifdef _FXCG
    dtext(1,2, C_BLUE,"Shortcuts OPTN=Toolbox, EXIT=Back, MENU=Home");
    dtext(1,20,C_BLUE, "From home, type OPTN to display this help");
    dtext(1,38,C_BLUE, "type MENU or DEL to leave Upsilon.");
    dtext(1,56,C_BLACK,"Save state, from Home type STO (above AC/ON)");
    dtext(1,74,C_BLACK,"You can manage and exchange Upsilon records");
    dtext(1,92,C_BLACK,"or restore states: from home, type VARS");
    dtext(1,110,C_BLACK,"To reset Upsilon state: from Casio main MENU");
    dtext(1,128,C_BLACK,"select Memory, type F2, then erase nwstore.nws");
#else
    dtext(1,2, C_BLUE,"Shortcuts menu=Toolbox, esc=Back, doc=Home");
    dtext(1,20,C_BLUE ,"From home, type ON or doc to leave Upsilon");
    dtext(1,38,C_BLUE ,"type Ctrl K to shutdown screen, ON to restore.");
    dtext(1,56,C_BLACK,"Save state, from Home type ctrl VAR then 1 to 9");
    dtext(1,74,C_BLACK,"You can manage and exchange Upsilon records");
    dtext(1,92,C_BLACK,"or restore states: from home, type VARS");
    dtext(1,110,C_BLACK,"To reset Upsilon state: from Home 2");
    dtext(1,128,C_BLACK,"erase nwstore.nws in My documents");
    os_wait_1ms(100);
#endif
    dtext(1,150,C_BLACK,"Upsilon: a fork of Epsilon (c) Numworks");
    dtext(1,168,C_BLACK,"https://getupsilon.web.app/");
    dtext(1,186,C_BLACK,"Persistance and host filesystem (c) B. Parisse");
    dtext(1,204,C_BLACK,"creativecommons.org license CC BY-NC-SA 4.0");
  }
  dupdate();
  do_getkey();
}

extern "C" void host_filemanager();
void host_filemanager(){
  bool fr=GlobalPreferences::sharedGlobalPreferences()->language()==I18n::Language(1);
  int posnw=0,startnw=0,poshost=0,starthost=0;
  bool nw=true,reload=true;
  std::vector<file> v,w;
#ifdef NSPIRE_NEWLIB
  std::string hostdir="/documents/";
#else
#ifdef __EMSCRIPTEN__
  std::string hostdir="/data/";
#else
  std::string hostdir="/";
#endif
#endif
  bool onlypy=true;
  for (;;){
    if (reload){
      nw_scripts(v,onlypy?".py":0); 
      sort(v.begin(),v.end(),filesort);
#ifdef NSPIRE_NEWLIB
      host_scripts(w,hostdir.c_str(),onlypy?".py.tns":0);
#else
      host_scripts(w,hostdir.c_str(),onlypy?".py":0);
#endif
      sort(w.begin(),w.end(),filesort);
      reload=false;
    }
    dclear(C_WHITE);
    dtext(1,1, C_BLACK,fr?"EXIT: quitte, touches 1 a 9: charge etat depuis flash":"EXIT: leave; key 1 to 9: load state from file");
#ifdef _FXCG
    dtext(1,17,C_BLACK,fr?"Curseur: deplace, /: rep. racine, OPTN: fich. tous/py":"Cursor keys: move, /: rootdir, OPTN: all/py files");
#else
#ifdef __EMSCRIPTEN__
    dtext(1,17,C_BLACK,fr?"Curseur: deplace, Toolbox: fich. tous/py":"Cursor keys: move, Toolbox: all/py files");
#else
    dtext(1,17,C_BLACK,fr?"Touches curseur: deplace, /: rep. racine":"Cursor keys: move, /: rootdir");
#endif
#endif
    dtext(1,33,C_BLACK,fr?"EXE ou STO: copie selection de/vers l'hote":"EXE or STO key: copy selection from/to host");
    dtext(1,49,C_BLACK,("Upsilon records             Host "+hostdir).c_str());
    int nitems=9;
    if (posnw<0)
      posnw=v.size()-1;
    if (posnw>=int(v.size()))
      posnw=0;
    if (posnw<startnw || posnw>startnw+nitems)
      startnw=posnw-4;
    if (startnw>=int(v.size())-nitems)
      startnw=v.size()-nitems;
    if (startnw<0)
      startnw=0;
    if (v.empty())
      nw=false;
    for (int i=0;i<=nitems;++i){
      int I=i+startnw;
      if (I>=int(v.size()))
	break;
      dtext(1,65+16*i,(nw && I==posnw)?C_RED:C_BLACK,v[I].s.c_str());
      char buf[256];
      sprintf(buf,"%i",v[I].length);
      dtext(90,65+16*i,(nw && I==posnw)?C_RED:C_BLACK,buf);
    }
    if (w.empty())
      nw=true;
    if (poshost<0)
      poshost=w.size()-1;
    if (poshost>=int(w.size()))
      poshost=0;
    if (poshost<starthost || poshost>starthost+nitems)
      starthost=poshost-4;
    if (starthost>=int(w.size())-nitems)
      starthost=w.size()-nitems;
    if (starthost<0)
      starthost=0;
    for (int i=0;i<=nitems;++i){
      int I=i+starthost;
      if (I>=int(w.size()))
	break;
      std::string fname=w[I].s;
      if (fname.size()>16)
	fname=fname.substr(0,16)+"...";
      dtext(192,65+16*i,(!nw && I==poshost)?C_RED:C_BLACK,fname.c_str());
      if (w[I].isdir)
	dtext(154,65+16*i,(!nw && I==poshost)?C_RED:C_BLACK,"<dir>");
      else {
	char buf[256];
	sprintf(buf,"%i",w[I].length);
#ifdef _FXCG
	dtext(340,65+16*i,(!nw && I==poshost)?C_RED:C_BLACK,buf);
#else
	dtext(285,65+16*i,(!nw && I==poshost)?C_RED:C_BLACK,buf);
#endif
      }
    }    
    dupdate();
    int key=do_getkey();
    if (key==KEY_EXIT || key==KEY_MENU)
      break;
    if (key==KEY_OPTN || key=='\t'){
      onlypy=!onlypy;
      reload=true;
      continue;
    }
    if (key==KEY_DIV){
#ifdef NSPIRE_NEWLIB
      hostdir="/documents/";
#else
      hostdir="/";
#endif
      reload=true;
      continue;
    }
    if (key==KEY_DEL){
      if (!nw && w[poshost].isdir) // can not remove directory
	continue;
      dclear(C_WHITE);
      dtext(1,17,C_BLACK,nw?(fr?"Suppression imminente de l'enregistrement Upsilon":"About to suppress Upsilon record:"):(fr?"Suppression imminente du fichier hote":"About to suppress Host file:"));
      dtext(1,33,C_BLACK,(nw?v[posnw].s:w[poshost].s).c_str());
      dtext(1,49,C_BLACK,fr?"Tapez EXE/enter pour confirmer":"Press EXE/enter or OK to confirm");
      dupdate();
      int ev=do_getkey();
      if (ev!=KEY_EXE && ev!=KEY_CTRL_OK)
	continue;
      if (nw){
#if 1
	erase_record(v[posnw].s.c_str());
#else
	char buf[256];
	strcpy(buf,v[posnw].s.c_str());
	int l=strlen(buf)-4;
	buf[l]=0;
	Ion::Storage::sharedStorage()-> destroyRecordWithBaseNameAndExtension(buf,buf+l+1);
#endif
      }
      else
	remove((hostdir+w[poshost].s).c_str());
      reload=true;
    }
    if (key==KEY_LEFT){
      nw=true;
      continue;
    }
    if (key==KEY_RIGHT){
      nw=false;
      continue;
    }
    if (key==KEY_PLUS){
      if (nw)
	posnw+=5;
      else 
	poshost+=5;
      continue;
    }
    if (key==KEY_MINUS){
      if (nw)
	posnw-=5;
      else 
	poshost-=5;
      continue;
    }
    if (key==KEY_DOWN){
      if (nw)
	++posnw;
      else 
	++poshost;
      continue;
    }
    if (key==KEY_UP){
      if (nw)
	--posnw;
      else 
	--poshost;
      continue;
    }
    int autoexec = key==KEY_EXE || key==KEY_CTRL_OK;
    if (key==KEY_STORE || autoexec){
      if (nw && posnw>=0 && posnw<int(v.size())){
	size_t i=0;
	for (;i<w.size();++i){
	  if (v[posnw].s==w[i].s)
	    break;
	}
	if (i<w.size()){
	  dclear(C_WHITE);
	  if (w[i].isdir){
	    dtext(1,33,C_BLACK,fr?"Ne peut pas ecraser un repertoire de l'hote":"Can not overwrite a Host directory!");
	    dupdate();
	    do_getkey();
	    continue;
	  }
	  dtext(1,33,C_BLACK,fr?"Le script existe dans le repertoire hote!":"Script exists in Host directory!");
	  dtext(1,49,C_BLACK,fr?"Tapez EXE/enter pour confirmer l'ecrasement":"Press EXE/enter to confirm overwrite");
	  dupdate();
	  int k=do_getkey();
	  if (k!=KEY_EXE && k!=KEY_CTRL_OK)
	    continue;
	}
#ifdef _FXCG
	int l=gint_world_switch(GINT_CALL(copy_nw_to_host,v[posnw].s.c_str(),(hostdir+v[posnw].s).c_str()));
#else
	int l=copy_nw_to_host(v[posnw].s.c_str(),(hostdir+v[posnw].s).c_str());
#endif
	char buf2[]="0";
	buf2[0] = '0'+l;
	dclear(C_WHITE);
	dtext(1,65,C_BLACK,(hostdir+v[posnw].s).c_str());
	dtext(1,81,C_BLACK,buf2);
	dupdate();
	if (l!=0)
	  do_getkey();
	reload=true;
      }
      else if (!nw && poshost>=0 && poshost<int(w.size())){
	if (w[poshost].s==".._parent_dir"){
	  // one level up
	  for (int j=hostdir.size()-2;j>=0;--j){
	    if (hostdir[j]=='/'){
	      hostdir=hostdir.substr(0,j+1);
	      break;
	    }
	  }
	  reload=true;
	  continue;
	}
	// lookup if poshost is in directories
	if (w[poshost].isdir){
	  hostdir += w[poshost].s;
	  hostdir += "/";
	  reload=true;
	  continue;
	}
	size_t i;
	std::string fname=w[poshost].s;
#ifdef NSPIRE_NEWLIB
	if (fname.size()>4 && fname.substr(fname.size()-4,4)==".tns")
	  fname=fname.substr(0,fname.size()-4);
#endif
	for (i=0;i<v.size();++i){
	  if (fname==v[i].s)
	    break;
	}
	if (i<v.size()){
	  dclear(C_WHITE);
	  dtext(1,33,C_BLACK,fr?"Le script existe dans Upsilon!":"Script exists in Upsilon!");
	  dtext(1,49,C_BLACK,fr?"Tapez EXE/enter pour confirmer l'ecrasement":"Press EXE/enter to confirm overwrite");
	  dupdate();
	  int k=do_getkey();
	  if (k!=KEY_EXE && k!=KEY_CTRL_OK)
	    continue;
	}
	std::string nwname=fname;
	if (fname.size()>12){
	  dclear(C_WHITE);
	  dtext(1,33,C_BLACK,fr?"Nom de fichier trop long pour l'hote":"Host filename too long");
	  dtext(1,49,C_BLACK,fname.c_str());
	  dupdate();
	  do_getkey();
	  continue;
	}
	if (fname.size()>4 && fname.substr(fname.size()-4,4)==".nws")
	  confirm_load_state((hostdir+fname).c_str());
	else {
#ifdef _FXCG
	  gint_world_switch(GINT_CALL(copy_host_to_nw,(hostdir+fname).c_str(),nwname.c_str(),autoexec));
#else
	  copy_host_to_nw((hostdir+w[poshost].s).c_str(),nwname.c_str(),autoexec);
#endif
	}
	reload=true;
      }
    }
    if (key>=KEY_1 && key<=KEY_9){
#ifdef NSPIRE_NEWLIB
      char buf[]="nwstate0.nws.tns";
#else
      char buf[]="nwstate0.nws";
#endif
      buf[7]='1'+(key-KEY_1);
      confirm_load_state(buf);
      reload=true;
    }
  }
}
#endif // FXCG || NSPIRE
    
#ifdef HOME_DISPLAY_EXTERNALS
#include "../external/external_icon.h"
#include "../external/archive.h"
#include <string.h>
#endif

namespace Home {

Controller::ContentView::ContentView(Controller * controller, SelectableTableViewDataSource * selectionDataSource) :
  m_selectableTableView(controller, controller, &m_backgroundView, selectionDataSource, controller),
  m_backgroundView()
{
  m_selectableTableView.setVerticalCellOverlap(0);
  m_selectableTableView.setMargins(0, k_sideMargin, k_bottomMargin, k_sideMargin);
  static_cast<ScrollView::BarDecorator *>(m_selectableTableView.decorator())->verticalBar()->setMargin(k_indicatorMargin);
}

SelectableTableView * Controller::ContentView::selectableTableView() {
  return &m_selectableTableView;
}

void Controller::ContentView::drawRect(KDContext * ctx, KDRect rect) const {
  m_selectableTableView.drawRect(ctx, rect);
}

void Controller::ContentView::reloadBottomRow(SimpleTableViewDataSource * dataSource, int numberOfIcons, int numberOfColumns) {
  if (numberOfIcons % numberOfColumns) {
    /* We mark the missing icons on the last row as dirty. */
    for (int i = 0; i < numberOfColumns; i++) {
      if (i >= numberOfIcons % numberOfColumns) {
        markRectAsDirty(KDRect(dataSource->cellWidth()*i, dataSource->cellHeight(), dataSource->cellWidth(), dataSource->cellHeight()));
      }
    }
  }
}

BackgroundView * Controller::ContentView::backgroundView() {
  return &m_backgroundView;
}

int Controller::ContentView::numberOfSubviews() const {
  return 1;
}

View * Controller::ContentView::subviewAtIndex(int index) {
  assert(index == 0);
  return &m_selectableTableView;
}

void Controller::ContentView::layoutSubviews(bool force) {
  m_selectableTableView.setFrame(bounds(), force);
  m_backgroundView.setFrame(KDRect(0, Metric::TitleBarHeight, Ion::Display::Width, Ion::Display::Height-Metric::TitleBarHeight), force);
  m_backgroundView.updateDataValidity();
}

Controller::Controller(Responder * parentResponder, SelectableTableViewDataSource * selectionDataSource, ::App * app) :
  ViewController(parentResponder),
  m_view(this, selectionDataSource)
{
  m_app = app;
  for (int i = 0; i < k_maxNumberOfCells; i++) {
    m_cells[i].setBackgroundView(m_view.backgroundView());
  }

  m_view.backgroundView()->setDefaultColor(Palette::HomeBackground);


#ifdef HOME_DISPLAY_EXTERNALS
    int index = External::Archive::indexFromName("wallpaper.obm");
    if(index > -1) {
      External::Archive::File image;
      External::Archive::fileAtIndex(index, image);
      m_view.backgroundView()->setBackgroundImage(image.data);
    }
#endif
}

static constexpr Ion::Events::Event home_fast_navigation_events[] = {
    Ion::Events::Seven, Ion::Events::Eight, Ion::Events::Nine,
    Ion::Events::Four, Ion::Events::Five, Ion::Events::Six,
    Ion::Events::One, Ion::Events::Two, Ion::Events::Three,
    Ion::Events::Zero, Ion::Events::Dot, Ion::Events::EE
};

bool Controller::handleEvent(Ion::Events::Event event) {
#ifdef XCAS
  if (event == Ion::Events::Backspace || event==Ion::Events::Home){
    save_calc_history();
#ifdef _FXCG
    int l=gint_world_switch(GINT_CALL(save_state,storage_name));
    Ion::Simulator::FXCGMenuHandler::openMenu();
#endif
    save_state(storage_name);
#ifdef __EMSCRIPTEN__
#endif
#ifdef NSPIRE_NEWLIB
    lcd_init(SCR_TYPE_INVALID);
    refresh_osscr(); // NSPIRE_NEWLIB
    exit(0);
#endif
  }
  if (Ion::Storage::sharedStorage()->numberOfRecords() && event == Ion::Events::Sto){
    dclear(C_WHITE);
    dtext(1,1, C_BLACK, "Key 1 to 9: save state");
    dtext(1,17,C_BLACK,"to file nwstate1 to 9");
    dtext(1,33,C_BLACK,"Press any other key to cancel");
    dupdate();
    int key=do_getkey();
#ifdef NSPIRE_NEWLIB
    char buf[]="nwstate0.nws.tns";
#else
    char buf[]="nwstate0.nws";
#endif
    if (key>=KEY_1 && key<=KEY_9){
      buf[7]='1'+(key-KEY_1);
      dclear(C_WHITE);
      dtext(1,1, C_BLACK, "Saving");
      dtext(1,17,C_BLACK,buf);
      dupdate();
      save_calc_history();
#ifdef _FXCG
      int l=gint_world_switch(GINT_CALL(save_state,buf));
#else
      int l=save_state(buf);
#endif
      char buf2[]="00000";
      for (int pos=0;l;l/=10,++pos){
	buf2[sizeof(buf2)-1-pos] += l % 10;
      }
      dtext(1,33,C_BLACK,"Length");
      dtext(1,49,C_BLACK,buf2);
      dtext(1,65,C_BLACK,"Press any key");
      dupdate();
      do_getkey();
      ((App*)m_app)->redraw();
    }
  }
  if (event==Ion::Events::Toolbox){
    display_host_help();
    display_giac_license(true);
    do_getkey();
    ((App*)m_app)->redraw();    
  }
  if (event == Ion::Events::Var){
    host_filemanager();
    ((App*)m_app)->redraw();
  }
#endif // _FXCG || NSPIRE_NEWLIB
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    AppsContainer * container = AppsContainer::sharedAppsContainer();

    int index = selectionDataSource()->selectedRow()*k_numberOfColumns+selectionDataSource()->selectedColumn()+1;
#ifdef HOME_DISPLAY_EXTERNALS
    if (index >= container->numberOfApps()) {
      if (GlobalPreferences::sharedGlobalPreferences()->examMode() == GlobalPreferences::ExamMode::Dutch || GlobalPreferences::sharedGlobalPreferences()->examMode() == GlobalPreferences::ExamMode::NoSymNoText || GlobalPreferences::sharedGlobalPreferences()->examMode() == GlobalPreferences::ExamMode::NoSym) {
        App::app()->displayWarning(I18n::Message::ForbiddenAppInExamMode1, I18n::Message::ForbiddenAppInExamMode2);
      } else {
        External::Archive::File executable;
        if (External::Archive::executableAtIndex(index - container->numberOfApps(), executable)) {
          uint32_t res = External::Archive::executeFile(executable.name, ((App *)m_app)->heap(), ((App *)m_app)->heapSize());
          ((App*)m_app)->redraw();
          switch(res) {
            case 0:
              break;
            case 1:
              Container::activeApp()->displayWarning(I18n::Message::ExternalAppApiMismatch);
              break;
            case 2:
              Container::activeApp()->displayWarning(I18n::Message::StorageMemoryFull1);
              break;
            default:
              Container::activeApp()->displayWarning(I18n::Message::ExternalAppExecError);
              break;
          }
          return true;
        }
      }
    } else {
#endif
    ::App::Snapshot * selectedSnapshot = container->appSnapshotAtIndex(index);
    if (ExamModeConfiguration::appIsForbiddenInExamMode(selectedSnapshot->descriptor()->examinationLevel(), GlobalPreferences::sharedGlobalPreferences()->examMode())) {
      App::app()->displayWarning(I18n::Message::ForbiddenAppInExamMode1, I18n::Message::ForbiddenAppInExamMode2);
    } else {
      bool switched = container->switchTo(selectedSnapshot);
      assert(switched);
      (void) switched; // Silence compilation warning about unused variable.
    }
#ifdef HOME_DISPLAY_EXTERNALS
    }
#endif
    return true;
  }

  if (event == Ion::Events::Home || event == Ion::Events::Back) {
    return m_view.selectableTableView()->selectCellAtLocation(0, 0);
  }

  if (event == Ion::Events::Right && selectionDataSource()->selectedRow() < numberOfRows() - 1) {
    return m_view.selectableTableView()->selectCellAtLocation(0, selectionDataSource()->selectedRow() + 1);
  }
  if (event == Ion::Events::Left && selectionDataSource()->selectedRow() > 0) {
    return m_view.selectableTableView()->selectCellAtLocation(numberOfColumns() - 1, selectionDataSource()->selectedRow() - 1);
  }

  // Handle fast home navigation
  for(int i = 0; i < std::min((int) (sizeof(home_fast_navigation_events) / sizeof(Ion::Events::Event)), this->numberOfIcons()); i++) {
    if (event == home_fast_navigation_events[i]) {
      int row = i / k_numberOfColumns;
      int column = i % k_numberOfColumns;
      // Get if app is already selected
      if (selectionDataSource()->selectedRow() == row && selectionDataSource()->selectedColumn() == column) {
        // If app is already selected, launch it
        return handleEvent(Ion::Events::OK);
      }
      // Else, select the app
      return m_view.selectableTableView()->selectCellAtLocation(column, row);
    }
  }

  return false;
}

void Controller::didBecomeFirstResponder() {
  if (selectionDataSource()->selectedRow() == -1) {
    selectionDataSource()->selectCellAtLocation(0, 0);
  }
  Container::activeApp()->setFirstResponder(m_view.selectableTableView());
}

void Controller::viewWillAppear() {
  KDIonContext::sharedContext()->zoomInhibit = true;
  KDIonContext::sharedContext()->updatePostProcessingEffects();
}

void Controller::viewDidDisappear() {
  KDIonContext::sharedContext()->zoomInhibit = false;
  KDIonContext::sharedContext()->updatePostProcessingEffects();
}

View * Controller::view() {
  return &m_view;
}

int Controller::numberOfRows() const {
  return ((numberOfIcons() - 1) / k_numberOfColumns) + 1;
}

int Controller::numberOfColumns() const {
  return k_numberOfColumns;
}

KDCoordinate Controller::cellHeight() {
  return k_cellHeight;
}

KDCoordinate Controller::cellWidth() {
  return k_cellWidth;
}

HighlightCell * Controller::reusableCell(int index) {
  return &m_cells[index];
}

int Controller::reusableCellCount() const {
  return k_maxNumberOfCells;
}

void Controller::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  AppCell * appCell = (AppCell *)cell;
  AppsContainer * container = AppsContainer::sharedAppsContainer();
  int appIndex = (j * k_numberOfColumns + i) + 1;
  if (appIndex >= container->numberOfApps()) {
#ifdef HOME_DISPLAY_EXTERNALS
    External::Archive::File app_file;


    if (External::Archive::executableAtIndex(appIndex - container->numberOfApps(), app_file)) {
      char temp_name_buffer[100];
      strlcpy(temp_name_buffer, app_file.name, 94);
      strlcat(temp_name_buffer, ".icon", 99);

      int img_index = External::Archive::indexFromName(temp_name_buffer);

      if (img_index != -1) {
        External::Archive::File image_file;
        if (External::Archive::fileAtIndex(img_index, image_file)) {
          // const Image* img = new Image(55, 56, image_file.data, image_file.dataLength);
          appCell->setExtAppDescriptor(app_file.name, image_file.data, image_file.dataLength);
        } else {
          appCell->setExtAppDescriptor(app_file.name, ImageStore::ExternalIcon);
        }
      } else {
        appCell->setExtAppDescriptor(app_file.name, ImageStore::ExternalIcon);
      }

      appCell->setVisible(true);
    } else {
      appCell->setVisible(false);
    }
#else
    appCell->setVisible(false);
#endif
  } else {
    appCell->setVisible(true);
    ::App::Descriptor * descriptor = container->appSnapshotAtIndex(PermutedAppSnapshotIndex(appIndex))->descriptor();
    appCell->setAppDescriptor(descriptor);
  }
}

int Controller::numberOfIcons() const {
  AppsContainer * container = AppsContainer::sharedAppsContainer();
  assert(container->numberOfApps() > 0);
#ifdef HOME_DISPLAY_EXTERNALS
  return container->numberOfApps() - 1 + External::Archive::numberOfExecutables();
#else
  return container->numberOfApps() - 1;
#endif
}

void Controller::tableViewDidChangeSelection(SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) {
  if (withinTemporarySelection) {
    return;
  }
  /* To prevent the selectable table view to select cells that are unvisible,
   * we reselect the previous selected cell as soon as the selected cell is
   * unvisible. This trick does not create an endless loop as we ensure not to
   * stay on a unvisible cell and to initialize the first cell on a visible one
   * (so the previous one is always visible). */
  int appIndex = (t->selectedColumn()+t->selectedRow()*k_numberOfColumns)+1;
  if (appIndex >= this->numberOfIcons()+1) {
    t->selectCellAtLocation((this->numberOfIcons()%k_numberOfColumns)-1, (this->numberOfIcons() / k_numberOfColumns));
  }
}

void Controller::tableViewDidChangeSelectionAndDidScroll(SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) {
  /* If the number of apps (including home) is != 3*n+1, when we display the
   * lowest icons, the other(s) are empty. As no icon is thus redrawn on the
   * previous ones, the cell is not cleaned. We need to redraw a white rect on
   * the cells to hide the leftover icons. Ideally, we would have redrawn all
   * the background in white and then redraw visible cells. However, the
   * redrawing takes time and is visible at scrolling. Here, we avoid the
   * background complete redrawing but the code is a bit
   * clumsy. */
  if (t->selectedRow() == numberOfRows()-1) {
    m_view.reloadBottomRow(this, this->numberOfIcons(), k_numberOfColumns);
  }
}

SelectableTableViewDataSource * Controller::selectionDataSource() const {
  return App::app()->snapshot();
}

}

#include "calculation_store.h"
#include "../shared/poincare_helpers.h"
#include "../global_preferences.h"
#include <poincare/rational.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>
#include "../exam_mode_configuration.h"
#include <assert.h>
#include "history_view_cell.h"

typedef const char * (*caseval_t)(const char *);
typedef int (*main_t)(void);
extern "C" caseval_t caseval;
caseval_t caseval= 0;

#ifdef XCAS
extern "C" int calculator;
extern "C" void restore_calc_history();
extern "C" int save_state(const char * fname);
extern "C" int save_calc_history();

#include <apps/shared/global_context.h>
static KDCoordinate dummyHeight(::Calculation::Calculation * c, bool expanded) {
  // Poincare::Expression e=c->exactOutput();
  bool b;
  Poincare::Layout l=c->createExactOutputLayout(&b);
  int h=0,w=0;
  if (1 || b){
    h=l.layoutSize().height();
    w=l.layoutSize().width();
  }
  l=c->createInputLayout();
  KDSize s=l.layoutSize();
  if (s.width()+w>
#ifdef _FXCG
      396-50
#else
      320-50
#endif
      )
    h += s.height();
  else if (s.height()>h)
    h=s.height();
  const int bordersize=14;
  h+=bordersize;
  const int maxheight=160;
  if (h>maxheight)
    return maxheight;
  return h;
}
 

extern void * last_calculation_history;
void * last_calculation_history=0;
const char * retrieve_calc_history();
std::string retrieve_functions();
void * storage_address(); // ion/src/simulator/shared/platform_info.cpp

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>


#ifdef _FXCG
#include "../../ion/src/simulator/fxcg/platform.h"
#include "../../ion/src/simulator/fxcg/menuHandler.h"
#include <gint/bfile.h>
#include <gint/display-cg.h>
#include <gint/gint.h>
#include <gint/display.h>
#include <gint/keyboard.h>
inline int do_getkey(){
  return getkey().key;
}

// reload=0 check, 1=check fnd reload, 2=force reload
int chk_ram2M(const char * filename,int reload){
  unsigned ram2Msize=0;
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  //bool is_emulator = !memcmp((void *)0xa001ffd0, "\xff\xff\xff\xff\xff\xff\xff\xff", 8);
  // emulator address 0x88200000, calculator address 0xac200000
  unsigned ram2Maddr, ram2Mendaddr;
  if (is_emulator)
    ram2Maddr=0x88200000;
  else 
    ram2Maddr=0xac200000;
#if 0
  unsigned pi=*(unsigned *)(ram2Maddr+4);
  if (reload==0 && pi!=0x31415927)
    return 0;
#endif
  ram2Mendaddr=ram2Maddr+ram2Msize;
  unsigned * ram2M=(unsigned *) ram2Maddr,*ram2Mend=(unsigned *) ram2Mendaddr;
  const unsigned chkinit=0x12345678;
  unsigned chk=chkinit,*ptr;
  if (ram2Msize && reload<2){
    for (ptr=ram2M;ptr<ram2Mend;ptr+=4){
      chk ^= (ptr[0] ^ ptr[1] ^ptr[2] ^ ptr[3]);
    }
    if (chk==*ptr)
      return 1; // proba to return true with corruption is tiny, about 1/4e9
    if (!reload)
      return 0;
  }
  // load ram part, filename="xcas.882" or "xcas.ac2"
  int nchar=strlen(filename);
  if (nchar<5 || filename[nchar-4]!='.' || filename[nchar-1]!='2')
    return -1;
  char pFile[nchar+1];
  strcpy(pFile,filename);
  if (is_emulator){
    pFile[nchar-3]='8';
    pFile[nchar-2]='8';
  }
  else {
    pFile[nchar-3]='a';
    pFile[nchar-2]='c';
  }
  FILE * f=fopen(pFile,"rb");
  if (!f)
    return -2; // nothing to load
  unsigned size=fread(ram2M,1,0x1e0000,f);
  fclose(f);
  ram2Msize=4*((size+3)/4);
  ram2Mendaddr=ram2Maddr+ram2Msize;
  ram2Mend=(unsigned *) ram2Mendaddr;
  // wait a little?
  // wait_1ms(100);
  // compute chksum
  chk=chkinit;
  for (ptr=ram2M;ptr<ram2Mend;ptr+=4){
    chk ^= (ptr[0] ^ ptr[1] ^ptr[2] ^ ptr[3]);
  }
  *ptr=chk; // save chksum
  return 1;
}

#endif // _FXCG

#ifdef NSPIRE_NEWLIB
#include "../../ion/src/simulator/nspire/k_csdk.h"
#endif

#ifdef __EMSCRIPTEN__
#include "../../ion/src/simulator/web/web.h"
extern "C" void copy_to_fs(const char * filename,const char * data,size_t length);
const char * em_caseval(const char * input){
  static std::string S="";
  char output[4096]; // must be the same in EM_ASM!
  EM_ASM({
      //var docaseval = Xcas.cwrap('nws_caseval', 'string', ['string']);
      var value = UTF8ToString($0);
      value = value.replace(/%22/g, '\"');
      var s; var err;
      console.log('caseval in=',value);
      try {
	s = docaseval(value);
      } catch (err){
	console.log('caseval error',err);
	s = 'error';
      }
      console.log('caseval out=',s);
      stringToUTF8(s,$1,4096);
    },input,output);
  for (int i=0;i<int(strlen(output))-6;++i){
    if (strncmp(output+i,"sqrt(",5)==0){ // replace by √
      output[i]=0xe2;
      output[i+1]=0x88;
      output[i+2]=0x9a;
      output[i+3]=' ';
    }
  }
  S=output;
  if (S.substr(0,6)=="write "){
    copy_to_fs("never","reached",7);
    save_calc_history();
    save_state("");
  }
  return S.c_str();
}
#endif

void display_giac_license(bool chkcas){
  dclear(C_WHITE);
  if (GlobalPreferences::sharedGlobalPreferences()->language()==I18n::Language(1)){
    if (chkcas && !caseval){
      dtext(1,18, C_BLACK,"Xcas n'est pas installe");
    }
    else {
      dtext(1,18, C_BLACK,"L'app. Calculs effectue les calculs avec");
      dtext(1,36,C_BLUE,"Xcas version light 1.9 (c) B. Parisse et al");
      dtext(1,54,C_BLUE,"  Universite Grenoble Alpes");
      dtext(1,72,C_BLACK,"License GPL3 avec une exception permettant les");
      dtext(1,90,C_BLACK,"appels depuis des logiciels de calculatrices");
      dtext(1,108,C_BLACK,"et emulateurs sous licence CC BY-NC-SA 4.0");
      dtext(1,126,C_BLACK,"pour un usage personnel et non commercial.");
#ifdef NSPIRE_NEWLIB
      dtext(1,154,C_BLACK,"Effacez xcasnws.tns pour empecher ces appels.");
#else
      dtext(1,154,C_BLACK,"Effacez xcas.ac2/882 pour empecher ces appels.");
#endif
      dtext(1,181,C_BLUE,"Utilisez KhiCAS pour avoir un CAS plus complet!");
    }
  } else {
    if (!caseval){
      dtext(1,18, C_BLACK,"Xcas is not installed");
    }
    else {
      dtext(1,18, C_BLACK,"The calculation app will evaluate with");
      dtext(1,36,C_BLUE,"Xcas light 1.9 version (c) B. Parisse");
      dtext(1,54,C_BLUE,"and others, Universite Grenoble Alpes");
      dtext(1,72,C_BLUE,"Released under the GPL with an exception");
      dtext(1,90,C_BLACK,"allowing inter-calls on calculators [emulators]");
      dtext(1,108,C_BLACK,"with CC BY-NC-SA 4.0 licensed software");
      dtext(1,126,C_BLACK,"for personal non commercial use only.");
#ifdef NSPIRE_NEWLIB
      dtext(1,154,C_BLACK,"Remove xcasnws.tns to revert this.");
#else
      dtext(1,154,C_BLACK,"Remove xcas.ac2/882 to revert this.");
#endif
      dtext(1,181,C_BLUE,"Switch to KhiCAS for more CAS features!");
    }
  }
  dtext(1,199,C_BLUE,"www-fourier.univ-grenoble-alpes.fr/~parisse");
  dupdate();
}

void restore_calc_history(){
#ifdef __EMSCRIPTEN__
  int i=EM_ASM_INT({
      console.log('restore_calc_history UI.ready=',UI.ready);
      if (UI.ready)
	return 1;
      window.setTimeout(ccall, 250,'restore_calc_history','v');
      return 0;
    });
  if (i==0)
    return ;
  std::string s=retrieve_functions();
  if (caseval && !s.empty())
    caseval(s.c_str());
#endif
  const char * buf =retrieve_calc_history();
  if (buf){
    Shared::GlobalContext globalContext;
    char * ptr=(char *)buf;
    for (;*ptr;){
      for (;*ptr;++ptr){
	if (*ptr=='\n')
	  break;
      }
      char c=*ptr;
      *ptr=0;
      if (ptr>buf)
	((Calculation::CalculationStore * )last_calculation_history)->push(buf,&globalContext,dummyHeight);
      *ptr=c;
      ++ptr;
      buf=ptr;
    }
  }
}  

#endif // XCAS


using namespace Poincare;
using namespace Shared;

namespace Calculation {
  
CalculationStore::CalculationStore(char * buffer, int size) :
  m_buffer(buffer),
  m_bufferSize(size),
  m_calculationAreaEnd(m_buffer),
  m_numberOfCalculations(0),
  m_trashIndex(-1)
{
  assert(m_buffer != nullptr);
  assert(m_bufferSize > 0);
#ifdef XCAS
  if (last_calculation_history==0){
    // find address for caseval
#ifdef __EMSCRIPTEN__
    caseval=em_caseval;
    // display_giac_license(true);
#endif
#ifdef NSPIRE_NEWLIB
    caseval=(caseval_t) 0;
    // for nspire, int nl_exec(const char *prgm_path, int argsn, char *args[]), call nl_set_resident() in xcas.tns
    char addrname[]="xcasaddr.txt.tns";
    char xcas_tns[]="/documents/ndless/xcasnws.tns";
    char xcas_tns1[]="/exammode/usr/ndless/xcasnws.tns";
    FILE * f=fopen(addrname,"r");
    if (!f){
      const char * xcas_exec=xcas_tns;
      f=fopen(xcas_exec,"rb");
      if (!f){
	xcas_exec=xcas_tns1;
	f=fopen(xcas_exec,"rb");
      }
      if (f){
	fclose(f);
	// run main() of Xcas light as a resident program
	// this should write the file xcasaddr.txt.tns
	char * args[]={0};
	display_giac_license(false);
	unsigned res=nl_exec(xcas_exec,0,args);
	do_getkey();
	f=fopen(addrname,"r");
      }
    }
    if (f){
      unsigned u1,u2,u3,u4; // u1 chk addr begin, u2 size, u3 chksum, u4 run_caseval
      fscanf(f,"%u %u %u %u",&u1,&u2,&u3,&u4);
      if (!feof(f)){
	int lang=0;
	fscanf(f,"%i",&lang);
	GlobalPreferences::sharedGlobalPreferences()->setLanguage(I18n::Language(lang));
      }
      fclose(f);
      unsigned * ptr=(unsigned *) u1;
      unsigned chk=0x31415927;
      for (unsigned i=0;i<u2;i+=4,++ptr){
	chk ^= *ptr;
      }
      if (chk!=u3){ // mismatch, recreate addrname
	char * args[]={0};
	int res=nl_exec(xcas_tns,0,args);
	f=fopen(addrname,"r");
	if (f){
	  fscanf(f,"%u %u %u %u",&u1,&u2,&u3,&u4);
	  fclose(f);
	  ptr=(unsigned *) u1;
	  chk=0x31415927;
	  for (unsigned i=0;i<u2;i+=4,++ptr){
	    chk ^= *ptr;
	  }
	}
      }
      if (chk==u3){ // ok, set caseval
	caseval=(caseval_t) u4;
      }
    }
#endif

#ifdef _FXCG
    // init xcas.ac2 or xcas.882
    bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
    uint32_t stack;
    __asm__("mov r15, %0" : "=r"(stack));
    bool prizmoremu=stack<0x8c000000;
    if (is_emulator || !prizmoremu){
      display_giac_license(false);
      int res=chk_ram2M("xcas.ac2",2);
      //do_getkey();
      if (res==1){
	if (calculator==1)
	  calculator=2; // use Python heap in static ram, not at 0x8c000000
	unsigned * ptr=(unsigned *)0xac200000;
	dclear(C_WHITE);
	dtext(1,2, C_BLACK,"Overlay loaded");
	if (prizmoremu)
	  ptr=(unsigned *)0x88200000;
	if (ptr[1]==0x31415927){
	  dtext(1,20, C_BLACK,"Initializing overlay");
	  main_t giac_main=(main_t) ptr[0];
	  unsigned * caseval_ptr=(unsigned *) giac_main();
	  dtext(1,38, C_BLACK,"Initialization done");
	  if (caseval_ptr){
	    dtext(1,56, C_BLACK,"caseval ptr address");
	    char buf[256]; 
	    sprintf(buf,"%x",(unsigned)caseval_ptr[0]);
	    dtext(1,84, C_BLACK,buf);
	    caseval=(caseval_t) caseval_ptr[0];
	    if (0){
	      // checking
	      const char * res=caseval("sin(pi/3)");
	      dtext(1,102,C_BLACK,res);
	      res=caseval("factor(x^4-1)");
	      dtext(1,122,C_BLACK,res);
	      res=caseval("integrate(x*sin(x))");
	      dtext(1,142,C_BLACK,res);
	    }
	  }
	}
	//dupdate(); getkey();
      }
    }
#endif
    last_calculation_history=(void *) this;
    // restore from scriptstore
    restore_calc_history();
  }
  std::string s=retrieve_functions();
  if (caseval && !s.empty())
    caseval(s.c_str());
#endif // XCAS
}

// Returns an expiring pointer to the calculation of index i, and ignore the trash
ExpiringPointer<Calculation> CalculationStore::calculationAtIndex(int i) {
#ifdef XCAS
  last_calculation_history=(void *) this;
#endif
  if (m_trashIndex == -1 || i < m_trashIndex) {
    return realCalculationAtIndex(i);
  } else {
    return realCalculationAtIndex(i + 1);
  }
}

// Returns an expiring pointer to the real calculation of index i
ExpiringPointer<Calculation> CalculationStore::realCalculationAtIndex(int i) {
  assert(i >= 0 && i < m_numberOfCalculations);
  // m_buffer is the address of the oldest calculation in calculation store
  Calculation * c = (Calculation *) m_buffer;
  if (i != m_numberOfCalculations-1) {
    // The calculation we want is not the oldest one so we get its pointer
    c = *reinterpret_cast<Calculation**>(addressOfPointerToCalculationOfIndex(i+1));
  }
  return ExpiringPointer<Calculation>(c);
}

// Pushes an expression in the store
ExpiringPointer<Calculation> CalculationStore::push(const char * text, Context * context, HeightComputer heightComputer) {
  static int xcasmode=1;
  bool xcascmd=false;
  if (text[0]=='x' && text[1]=='c' && text[2]=='a' && text[3]=='s'){
    xcascmd=true;
    if (text[4]=='=')
      xcasmode=(text[5]-'0');
#ifdef _FXCG
    if (1){
      char buf[128];
      dclear(C_WHITE);
      sprintf(buf,"xcasmode %i",xcasmode);
      dtext(1,1,C_BLACK,buf);
      dupdate();
      //getkey();
    }
#endif    
  }
  emptyTrash();
  /* Compute ans now, before the buffer is updated and before the calculation
   * might be deleted */
  Expression ans = ansExpression(context);

  /* Prepare the buffer for the new calculation
   * The minimal size to store the new calculation is the minimal size of a calculation plus the pointer to its end */
  int minSize = Calculation::MinimalSize() + sizeof(Calculation *);
  assert(m_bufferSize > minSize);
  while (remainingBufferSize() < minSize) {
    // If there is no more space to store a calculation, we delete the oldest one
    deleteOldestCalculation();
  }

  // Getting the adresses of the limits of the free space
  char * beginingOfFreeSpace = (char *)m_calculationAreaEnd;
  char * endOfFreeSpace = beginingOfMemoizationArea();
  char * previousCalc = beginingOfFreeSpace;

  // Add the beginning of the calculation
  {
    /* Copy the begining of the calculation. The calculation minimal size is
     * available, so this memmove will not overide anything. */
    Calculation newCalc = Calculation();
    size_t calcSize = sizeof(newCalc);
    memcpy(beginingOfFreeSpace, &newCalc, calcSize);
    beginingOfFreeSpace += calcSize;
  }

  /* Add the input expression.
   * We do not store directly the text entered by the user because we do not
   * want to keep Ans symbol in the calculation store. */
  const char * inputSerialization = beginingOfFreeSpace;
  {
    Expression input = Expression::Parse(text, context).replaceSymbolWithExpression(Symbol::Ans(), ans);
    if (!pushSerializeExpression(input, beginingOfFreeSpace, &endOfFreeSpace)) {
      /* If the input does not fit in the store (event if the current
       * calculation is the only calculation), just replace the calculation with
       * undef. */
      return emptyStoreAndPushUndef(context, heightComputer);
    }
    beginingOfFreeSpace += strlen(beginingOfFreeSpace) + 1;
  }

  // Compute and serialize the outputs
  /* The serialized outputs are:
   * - the exact ouput
   * - the approximate output with the maximal number of significant digits
   * - the approximate output with the displayed number of significant digits */
  {
    // Outputs hold exact output, approximate output and its duplicate
    constexpr static int numberOfOutputs = Calculation::k_numberOfExpressions - 1;
    Expression outputs[numberOfOutputs] = {Expression(), Expression(), Expression()};
    bool done=false;
#ifdef XCAS
    const int BUFSIZE=1024;
    if (!xcascmd && xcasmode && caseval && strlen(text)<BUFSIZE-32){
#ifdef _FXCG
      if (0){
	char buf[128];
	dclear(C_WHITE);
	dtext(1,2, C_BLACK,text);
	sprintf(buf,"len %i",strlen(text));
	dtext(1,20,C_BLACK,buf);
	for (size_t i=0;i<10 && i<strlen(text);++i){
	  sprintf(buf,"%i: 0x%x",i,(unsigned)text[i]);       
	  dtext(1,40+i*18,C_BLACK,buf);
	}
	dupdate();
	getkey();
      }
#endif
      // link to giac
      Preferences * preferences = Preferences::sharedPreferences();
      if (preferences->angleUnit() == Preferences::AngleUnit::Radian)
	caseval("angle_radian:=1");
      else
	caseval("angle_radian:=0");
      char * buf=(char *)malloc(BUFSIZE);
      if (buf){
	strcpy(buf,"add_autosimplify(");
	strcat(buf,text);
	buf[strlen(buf)+1]=0;
	buf[strlen(buf)]=')';
	bool isunit_seq=false;
	for (size_t i=0;i<strlen(buf);++i){
	  if (i && buf[i]=='('){
	    int j;
	    for (j=i-1;j>=0;--j){
	      char ch=buf[j];
	      if (!isalpha(ch) || (ch>='0' && ch<='9'))
		break;
	    }
	    char Name[32]={0}; // max size for identifiers is 16
	    ++j;
	    if (j<int(i) && i-j<16 && isalpha(buf[j])){
	      strncpy(Name,buf+j,i-j);
	      strcat(Name,".seq");
	      // lookup for Name in storage
	      const unsigned char * ptr=(const unsigned char *)storage_address();
	      for (ptr+=4;;){ // skip header
		size_t L=ptr[1]*256+ptr[0]; 
		if (L==0) break; // not found
		char * name=(char *)ptr+2;
		if (strcmp(name,Name)==0){
		  isunit_seq=true; // revert to Upsilon eval
		  break;
		}
		ptr+=L;
	      }
	    }
	  }
	  if (buf[i]==(char)0xf0
	      && buf[i+1]==(char)0x9d
	      && buf[i+2]==(char)0x90
	      // && buf[i+3]==0xa2 // does not work with 0xa2 no idea why
	      ){
	    buf[i]=buf[i+2]=buf[i+3]=' ';
	    buf[i+1]='i';
	  }
	  if (buf[i]==(char)0xe2 && buf[i+1]==(char)0x88 && buf[i+2]==(char)0x9e){
	    buf[i]='i'; buf[i+1]='n'; buf[i+2]='f';
	  }
	  if (buf[i]==(char)0xe2 && buf[i+1]==(char)0x86 && buf[i+2]==(char)0x92){
	    buf[i]=' '; buf[i+1]='='; buf[i+2]='>';
	  }
	  if (buf[i]==(char)0xc3 && buf[i+1]==(char)0x97){
	    if (buf[i+2]==(char)0x5f){
	      isunit_seq=true; // revert to Upsilon eval
	      break;
	    }
	    else
	      buf[i]='*';
	    buf[i+1]=' ';
	  }
	  if (buf[i]==0x12)
	    buf[i]='(';
	  if (buf[i]==0x13)
	    buf[i]=')';
	}
	if (!isunit_seq){
#ifdef _FXCG
	  if (0){
	    dclear(C_WHITE);
	    dtext(1,20,C_BLACK,buf);
	    int shift=16;
	    for (size_t i=0;i<10 && i+shift<strlen(buf);++i){
	      char txt[128];
	      sprintf(txt,"%i: 0x%x",i+shift,(unsigned)buf[i+shift]);       
	      dtext(1,40+i*18,C_BLACK,txt);
	    }
	    dupdate();
	    getkey();
	  }
	  const char * outexact=(char const *) gint_world_switch(GINT_CALL(caseval,buf));
	  if (0){
	    dclear(C_WHITE);
	    dtext(1,200, C_RED,outexact);
	    dupdate();
	    do_getkey();
	  }
#else
	  const char * outexact=caseval(buf);
#endif
	  done=true;
	  if (strncmp(outexact,"function",8)==0){
	    outputs[1]=outputs[0]=Expression::Parse("function", context);
	    //done=false;
	    if (strlen(outexact)>9){
	      // extract function definition
	      outputs[0]=Expression::Parse(outexact+9, context);
	    }
	  }
	  else if (strlen(outexact)>9 && strncmp(outexact,"variable ",9)==0){
	    // extract var def
	    outputs[1]=outputs[0]=Expression::Parse(outexact+9, context);
	  }
	  else {
	    // parse exact answer
	    outputs[0]=Expression::Parse(outexact, context);
	    outputs[1]=outputs[0];
	  }
#ifdef _FXCG
	  if (0){
	    dclear(C_WHITE);
	    dtext(1,20, C_BLACK,"Done"); 
	    dupdate();
	    do_getkey();
	  }
#endif
	}
	free(buf);
      }
    }
#endif
    if (!done)
      PoincareHelpers::ParseAndSimplifyAndApproximate(inputSerialization, &(outputs[0]), &(outputs[1]), context, GlobalPreferences::sharedGlobalPreferences()->isInExamModeSymbolic() ? Poincare::ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition : Poincare::ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
    if (ExamModeConfiguration::exactExpressionsAreForbidden(GlobalPreferences::sharedGlobalPreferences()->examMode()) && outputs[1].hasUnit()) {
      // Hide results with units on units if required by the exam mode configuration
      outputs[1] = Undefined::Builder();
    }
    outputs[2] = outputs[1];
    int numberOfSignificantDigits = Poincare::PrintFloat::k_numberOfStoredSignificantDigits;
    for (int i = 0; i < numberOfOutputs; i++) {
      if (i == numberOfOutputs - 1) {
        numberOfSignificantDigits = Poincare::Preferences::sharedPreferences()->numberOfSignificantDigits();
      }
      if (!pushSerializeExpression(outputs[i], beginingOfFreeSpace, &endOfFreeSpace, numberOfSignificantDigits)) {
        /* If the exact/approximate output does not fit in the store (event if the
         * current calculation is the only calculation), replace the output with
         * undef if it fits, else replace the whole calculation with undef. */
        Expression undef = Undefined::Builder();
        if (!pushSerializeExpression(undef, beginingOfFreeSpace, &endOfFreeSpace)) {
          return emptyStoreAndPushUndef(context, heightComputer);
        }
      }
      beginingOfFreeSpace += strlen(beginingOfFreeSpace) + 1;
    }
  }
  // Storing the pointer of the end of the new calculation
  memcpy(endOfFreeSpace-sizeof(Calculation*),&beginingOfFreeSpace,sizeof(beginingOfFreeSpace));

  // The new calculation is now stored
  m_numberOfCalculations++;

  // The end of the calculation storage area is updated
  m_calculationAreaEnd += beginingOfFreeSpace - previousCalc;
  ExpiringPointer<Calculation> calculation = ExpiringPointer<Calculation>(reinterpret_cast<Calculation *>(previousCalc));
  /* Heights are computed now to make sure that the display output is decided
   * accordingly to the remaining size in the Poincare pool. Once it is, it
   * can't change anymore: the calculation heights are fixed which ensures that
   * scrolling computation is right. */
  calculation->setHeights(
      heightComputer(calculation.pointer(), false),
      heightComputer(calculation.pointer(), true));
  return calculation;
}

// Delete the calculation of index i
void CalculationStore::deleteCalculationAtIndex(int i) {
  if (m_trashIndex != -1) {
    emptyTrash();
  }
  m_trashIndex = i;
}

// Delete the calculation of index i, internal algorithm
void CalculationStore::realDeleteCalculationAtIndex(int i) {
  assert(i >= 0 && i < m_numberOfCalculations);
  if (i == 0) {
    ExpiringPointer<Calculation> lastCalculationPointer = realCalculationAtIndex(0);
    m_calculationAreaEnd = (char *)(lastCalculationPointer.pointer());
    m_numberOfCalculations--;
    return;
  }
  char * calcI = (char *)realCalculationAtIndex(i).pointer();
  char * nextCalc = (char *) realCalculationAtIndex(i-1).pointer();
  assert(m_calculationAreaEnd >= nextCalc);
  size_t slidingSize = m_calculationAreaEnd - nextCalc;
  // Slide the i-1 most recent calculations right after the i+1'th
  memmove(calcI, nextCalc, slidingSize);
  m_calculationAreaEnd -= nextCalc - calcI;
  // Recompute pointer to calculations after the i'th
  recomputeMemoizedPointersAfterCalculationIndex(i);
  m_numberOfCalculations--;
}

// Delete the oldest calculation in the store and returns the amount of space freed by the operation
size_t CalculationStore::deleteOldestCalculation() {
  char * oldBufferEnd = (char *) m_calculationAreaEnd;
  realDeleteCalculationAtIndex(numberOfCalculations()-1);
  char * newBufferEnd = (char *) m_calculationAreaEnd;
  return oldBufferEnd - newBufferEnd;
}

// Delete all calculations
void CalculationStore::deleteAll() {
  m_trashIndex = -1;
  m_calculationAreaEnd = m_buffer;
  m_numberOfCalculations = 0;
}

// Replace "Ans" by its expression
Expression CalculationStore::ansExpression(Context * context) {
  if (numberOfCalculations() == 0) {
    return Rational::Builder(0);
  }
  ExpiringPointer<Calculation> mostRecentCalculation = calculationAtIndex(0);
  /* Special case: the exact output is a Store/Equal expression.
   * Store/Equal expression can only be at the root of an expression.
   * To avoid turning 'ans->A' in '2->A->A' or '2=A->A' (which cannot be
   * parsed), ans is replaced by the approximation output when any Store or
   * Equal expression appears. */
  Expression e = mostRecentCalculation->exactOutput();
  bool exactOutputInvolvesStoreEqual = e.type() == ExpressionNode::Type::Store || e.type() == ExpressionNode::Type::Equal;
  if (mostRecentCalculation->input().recursivelyMatches(Expression::IsApproximate, context) || exactOutputInvolvesStoreEqual) {
    return mostRecentCalculation->approximateOutput(context, Calculation::NumberOfSignificantDigits::Maximal);
  }
  return mostRecentCalculation->exactOutput();
}

// Push converted expression in the buffer
bool CalculationStore::pushSerializeExpression(Expression e, char * location, char * * newCalculationsLocation, int numberOfSignificantDigits) {
  assert(*newCalculationsLocation <= m_buffer + m_bufferSize);
  bool expressionIsPushed = false;
  while (true) {
    size_t locationSize = *newCalculationsLocation - location;
    expressionIsPushed = (PoincareHelpers::Serialize(e, location, locationSize, numberOfSignificantDigits) < (int)locationSize-1);
    if (expressionIsPushed || *newCalculationsLocation >= m_buffer + m_bufferSize) {
      break;
    }
    *newCalculationsLocation = *newCalculationsLocation + deleteOldestCalculation();
    assert(*newCalculationsLocation <= m_buffer + m_bufferSize);
  }
  return expressionIsPushed;
}

void CalculationStore::emptyTrash() {
  if (m_trashIndex != -1) {
    realDeleteCalculationAtIndex(m_trashIndex);
    m_trashIndex = -1;
  }
}


Shared::ExpiringPointer<Calculation> CalculationStore::emptyStoreAndPushUndef(Context * context, HeightComputer heightComputer) {
  /* We end up here as a result of a failed calculation push. The store
   * attributes are not necessarily clean, so we need to reset them. */
  deleteAll();
  return push(Undefined::Name(), context, heightComputer);
}

// Recompute memoized pointers to the calculations after index i
void CalculationStore::recomputeMemoizedPointersAfterCalculationIndex(int index) {
  assert(index < m_numberOfCalculations);
  // Clear pointer and recompute new ones
  Calculation * c = realCalculationAtIndex(index).pointer();
  Calculation * nextCalc;
  while (index != 0) {
    nextCalc = c->next();
    memcpy(addressOfPointerToCalculationOfIndex(index), &nextCalc, sizeof(Calculation *));
    c = nextCalc;
    index--;
  }
}

}

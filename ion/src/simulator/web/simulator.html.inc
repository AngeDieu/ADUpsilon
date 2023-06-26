<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Upsilon CASworks</title>
    <style>
#include "calculator.css"
#include "simulator.css"
    </style>
  </head>
  <body>
    <div> <a href="casworks.html" target="_blank">Documentation</a> de la <a href="casworks.zip">calculatrice</a> mariant <a href="https://getupsilon.web.app/"  target="_blank">Upsilon</a> (<a href="https://creativecommons.org/licenses/by-nc-sa/4.0/deed.fr"  target="_blank">licence</a>) et <a href="https://xcas.univ-grenoble-alpes.fr"  target="_blank">Xcas</a> (<a href="https://www.gnu.org/licenses/quick-guide-gplv3.fr.html"  target="_blank">GPL3</a> avec exception). Merci à <a href="https://numworks.com"  target="_blank">Numworks</a> pour <a href="https://github.com/numworks/epsilon/tree/version-15"  target="_blank">Epsilon 15.5</a><br>
      Sauvegarde: touche Home depuis Home ou
      <button class="bouton" title="Cliquer ici pour sauvegarder l'état de la calculatrice avec le nom de fichier du champ texte qui suit" onclick="UI.save();">&#x1f4be;</button>
      <textarea cols="15" rows="1" id="outputfilename"  style="font-size:large" title="Nom de sauvegarde du fichier. Avec Firefox, pour pouvoir faire plusieurs sauvegardes sous le meme nom, cocher Toujours demander... dans Pr&eacute;f&eacute;rences, G&eacute;n&eacute;ral, T&eacute;l&eacute;chargements.">nwstate</textarea>

      <button id="action-fullscreen" onclick='if (document.body.className == "fullscreen") document.body.className = ""; else document.body.className = "fullscreen";' title="Bascule d'affichage en grand de l'écran de la calculatrice">Miroir</button>
      <button id="action-screenshot" onclick="CalcModule.downloadScreenshot();" title="Capture écran de la calculatrice">Capture</button>

      <form id="loadbutton_file">
	<label for="loadfileinput">Charger</label>
	<input id="loadfileinput" type="file" title="Cliquer ici pour charger un fichier vers Upsilon" onchange="UI.loadfile(this.files);this.value=''">
      </form>
    <div class="row">
      <div class="col-calculator">
        <div class="calculator-container">
          <img src="background.jpg" alt="NumWorks Calculator">
#include "calculator.html"
        </div>
      </div>
      <div class="col-fullscreen calculator-mirror">
        <canvas width="320" height="240"></canvas>
      </div>
    </div>

    <script src="epsilon.js"></script>
    <script src="xcas_ups.js"></script>    
    <script>
function $id(id) { return document.getElementById(id); }
#include "calculator.js"
      var CalcModule = {};
      var UI = {
	  save:function(){
	      var filename=$id("outputfilename").value;
	      if (filename.length<=4 || filename.substr(filename.length-4,4)!='.nws')
		  filename += '.nws';
	      CalcModule.ccall('save_calc_history','v');
	      var cp=CalcModule.cwrap('save_state','null',['string']);
	      cp(filename);
	  },
	  Datestart:Date.now(), // used by emcctime()
	  ready:false, // set to true when we may call caseval
	  detectmob: function () {
	      if (navigator.userAgent.match(/Android/i)
		  || navigator.userAgent.match(/webOS/i)
		  || navigator.userAgent.match(/iPhone/i)
		  || navigator.userAgent.match(/iPad/i)
		  || navigator.userAgent.match(/iPod/i)
		  || navigator.userAgent.match(/BlackBerry/i)
		  || navigator.userAgent.match(/Windows Phone/i)
		 ) return true;
	      else
		  return false;
	  },
	  loadfile:function (oFiles) {
	      var nFiles = oFiles.length;
	      for (var nFileId = 0; nFileId < nFiles; nFileId++) {
		  var filename=oFiles[nFileId].name;
		  console.log('load',filename);
		  // $id('outputfilename').innerHTML=UI.remove_extension(oFiles[nFileId].name);
		  var reader = new FileReader();
		  if (1){
		      reader.readAsArrayBuffer(oFiles[nFileId]);
		      var s;
		      reader.onloadend = function (e) {
			  s = e.target.result;
			  const view = new Uint8Array(s);			
			  console.log("load buffer",s.byteLength,view);
			  let cp=CalcModule.cwrap('copy_to_fs', 'null', ['string','array','number']);
			  cp(filename,view,s.byteLength);
		      }
		  }
		  else {
		      reader.readAsBinaryString(oFiles[nFileId]);
		      var s;
		      reader.onloadend = function (e) {
			  s = e.target.result;
			  console.log("load buffer",s.length,s);
			  let cp=CalcModule.cwrap('copy_to_fs', 'null', ['string','string','number']);
			  cp(filename,s,s.length);
		      }
		  }
	      }
	  },
      };
      window.onbeforeunload = (e) => {
	  var dialogText = "Etes-vous sur?";
	  return dialogText;
      };      
      Calculator(CalcModule);
      if (!UI.detectmob()) document.body.className = "fullscreen";
    </script>
    <!--<script async src="https://www.numworks.com/simulator/update.js"></script>-->
  </body>
</html>

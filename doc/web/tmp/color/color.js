// http://d.hatena.ne.jp/DECKS/20100907/1283843862
rgbTo16 = function(col){
	if (col.match(/^#/)) {
		return col;
	}
	return "#" + col.match(/\d+/g).map(function(a){ return ("0" + parseInt(a).toString(16)).slice(-2)}).join("");
}
rgbFrom16 = function(col){
	var ret = '';
	ret = ret + parseInt(col.substr(1,2), 16);
	ret = ret + ',';
	ret = ret + parseInt(col.substr(3,2), 16);
	ret = ret + ',';
	ret = ret + parseInt(col.substr(5,2), 16);
	return ret;
}

function affect_to_sample(element) {
	var r = parseInt($('#R').val()).toString(16);
	var g = parseInt($('#G').val()).toString(16);
	var b = parseInt($('#B').val()).toString(16);
	if (r.length == 1) {
		r = '0' + r;
	}
	if (g.length == 1) {
		g = '0' + g;
	}
	if (b.length == 1) {
		b = '0' + b;
	}
	var color = '#' + r + g + b;
	if (element == 'BG') {
		$('#'+element).css('background-color', color);
	}
	else {
		$('#'+element).css('color', color);
		if (element == 'Normal') {
			$('#Reverse').css('background-color', color);
		}
	}
}

function affect_to_inputbox(color) {
	var r = parseInt(color.substr(1, 2), 16);
	var g = parseInt(color.substr(3, 2), 16);
	var b = parseInt(color.substr(5, 2), 16);
	$('#R').val(r);
	$('#G').val(g);
	$('#B').val(b);
}

function affect_to_slider() {
	$('#sliderR').slider('value', $('#R').val());
	$('#sliderG').slider('value', $('#G').val());
	$('#sliderB').slider('value', $('#B').val());
}

function affect_from_preset(preset) {
	for(var key in preset) {
		if (key == 'name') {
			continue;
		}
		if (key == 'BG') {
			$('#'+key).css('background-color', preset[key]);
		}
		else {
			$('#'+key).css('color', preset[key]);
			if (key == 'Reverse') {
				$('#'+key).css('background-color', preset['Normal']);
			}
		}
	}
}

function affect_to_textbox() {
	var str = '';
	var bg = rgbFrom16(rgbTo16($('#BG').css('background-color')));
	str = str + "UseNormalBGColor=on\n";
	str = str + "VTColor=" + rgbFrom16(rgbTo16($('#Normal').css('color'))) + "," + bg + "\n";
	str = str + "EnableBoldAttrColor=on\n";
	str = str + "VTBoldColor=" + rgbFrom16(rgbTo16($('#Bold').css('color'))) + "," + bg + "\n";
	str = str + "EnableBlinkAttrColor=on\n";
	str = str + "VTBlinkColor=" + rgbFrom16(rgbTo16($('#Blink').css('color'))) + "," + bg + "\n";
	str = str + "EnableReverseAttrColor=on\n";
	str = str + "VTReverseColor=" + rgbFrom16(rgbTo16($('#Reverse').css('color'))) + "," + bg + "\n";
	str = str + "EnableURLColor=on\n";
	str = str + "URLColor=" + rgbFrom16(rgbTo16($('#URL').css('color'))) + "," + bg + "\n";
	str = str + "EnableANSIColor=on\n";
	str = str + "ANSIColor=";
	for (i=0; i<=15; i++) {
		str = str + i + ',' + rgbFrom16(rgbTo16($('#ANSI'+i).css('color'))) + ', ';
	}
	str = str.substr(0,str.length-2);
	$('#ini').text(str);
}

var TeraTerm = new Array();
TeraTerm['name'] = 'Tera Term';
TeraTerm['BG']      = '#FFFFF0';
TeraTerm['Normal']  = '#000000';
TeraTerm['Bold']    = '#0000FF';
TeraTerm['Blink']   = '#FF0000';
TeraTerm['Reverse'] = '#FFFFFF';
TeraTerm['URL']     = '#E4379C';
TeraTerm['ANSI0']   = '#000000';
TeraTerm['ANSI1']   = '#FF0000';
TeraTerm['ANSI2']   = '#00FF00';
TeraTerm['ANSI3']   = '#FFFF00';
TeraTerm['ANSI4']   = '#8080FF';
TeraTerm['ANSI5']   = '#FF00FF';
TeraTerm['ANSI6']   = '#00FFFF';
TeraTerm['ANSI7']   = '#FFFFFF';
TeraTerm['ANSI8']   = '#404040';
TeraTerm['ANSI9']   = '#C00000';
TeraTerm['ANSI10']  = '#00C000';
TeraTerm['ANSI11']  = '#C0C000';
TeraTerm['ANSI12']  = '#4040C0';
TeraTerm['ANSI13']  = '#C000C0';
TeraTerm['ANSI14']  = '#00C0C0';
TeraTerm['ANSI15']  = '#C0C0C0';

var xterm = new Array();
xterm['name'] = 'xterm';
xterm['BG']      = '#000000';
xterm['Normal']  = '#FFFFFF';
xterm['Bold']    = '#FFFFFF';
xterm['Blink']   = '#FFFFFF';
xterm['Reverse'] = '#000000';
xterm['URL']     = '#FFFFFF';
xterm['ANSI0']   = '#000000';
xterm['ANSI1']   = '#FF0000';
xterm['ANSI2']   = '#00FF00';
xterm['ANSI3']   = '#FFFF00';
xterm['ANSI4']   = '#5C5CFF';
xterm['ANSI5']   = '#FF00FF';
xterm['ANSI6']   = '#00FFFF';
xterm['ANSI7']   = '#FFFFFF';
xterm['ANSI8']   = '#7F7F7F';
xterm['ANSI9']   = '#CD0000';
xterm['ANSI10']  = '#00CD00';
xterm['ANSI11']  = '#CDCD00';
xterm['ANSI12']  = '#0000EE';
xterm['ANSI13']  = '#CD00CD';
xterm['ANSI14']  = '#00CDCD';
xterm['ANSI15']  = '#E5E5E5';

var PuTTY = new Array();
PuTTY['name'] = 'PuTTY';
PuTTY['BG']      = '#000000';
PuTTY['Normal']  = '#BBBBBB';
PuTTY['Bold']    = '#FFFFFF';
PuTTY['Blink']   = '#BBBBBB';
PuTTY['Reverse'] = '#000000';
PuTTY['URL']     = '#BBBBBB';
PuTTY['ANSI0']   = '#000000';
PuTTY['ANSI1']   = '#FF5555';
PuTTY['ANSI2']   = '#55FF55';
PuTTY['ANSI3']   = '#FFFF55';
PuTTY['ANSI4']   = '#5555FF';
PuTTY['ANSI5']   = '#FF55FF';
PuTTY['ANSI6']   = '#55FFFF';
PuTTY['ANSI7']   = '#FFFFFF';
PuTTY['ANSI8']   = '#555555';
PuTTY['ANSI9']   = '#BB0000';
PuTTY['ANSI10']  = '#00BB00';
PuTTY['ANSI11']  = '#BBBB00';
PuTTY['ANSI12']  = '#0000BB';
PuTTY['ANSI13']  = '#BB00BB';
PuTTY['ANSI14']  = '#00BBBB';
PuTTY['ANSI15']  = '#BBBBBB';


var TeraTerm23 = new Array();
TeraTerm23['name'] = 'Tera Term 2.3 + ANSI';
TeraTerm23['BG']      = '#FFFFFF';
TeraTerm23['Normal']  = '#000000';
TeraTerm23['Bold']    = '#00FFFF';
TeraTerm23['Blink']   = '#FFFF00';
TeraTerm23['Reverse'] = '#000000';
TeraTerm23['URL']     = '#000000';
TeraTerm23['ANSI0']   = '#000000';
TeraTerm23['ANSI1']   = '#FF0000';
TeraTerm23['ANSI2']   = '#00FF00';
TeraTerm23['ANSI3']   = '#FFFF00';
TeraTerm23['ANSI4']   = '#8080FF';
TeraTerm23['ANSI5']   = '#FF00FF';
TeraTerm23['ANSI6']   = '#00FFFF';
TeraTerm23['ANSI7']   = '#FFFFFF';
TeraTerm23['ANSI8']   = '#000000';
TeraTerm23['ANSI9']   = '#FF0000';
TeraTerm23['ANSI10']  = '#00FF00';
TeraTerm23['ANSI11']  = '#FFFF00';
TeraTerm23['ANSI12']  = '#8080FF';
TeraTerm23['ANSI13']  = '#FF00FF';
TeraTerm23['ANSI14']  = '#00FFFF';
TeraTerm23['ANSI15']  = '#FFFFFF';

var Preset = new Array(TeraTerm, xterm, PuTTY, TeraTerm23);

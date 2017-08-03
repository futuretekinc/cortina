/*-----------------------------------------------------------------------------
 * Net WEBDOC.H
 *
 * This module contains web pages and headers
 *-----------------------------------------------------------------------------
 */
#include <net.h>

char web_page[]={
	/* "<html><head><title>Simple HTTP Web Server</title></head>" */
	"<body><form method=\"post\" action=\"/upgrade.html\" enctype=\"multipart/form-data\"><center>\n\r"
	"<font color=blue face=verdana size=3><b>Firmware Upgrade</b></font>"
	"<p><table cellPadding=0 cellSpacing=10><tr><td align=right>File Path</td>"
	"<td>&nbsp;&nbsp;<input type=file name=files value=""></td></tr><tr><td>&nbsp;</td>"
	"<td><input type=\"submit\" value=\"Send\"></td></tr></table></form><hr>"
	"<ul><font face=Verdana color=red size=4>NOTICE !!</font></ul><br></center>"
	"<font face=Verdana size=1><li>Do not remove power and the LAN cable connected "
	"during the upgrading process. (Upgrade by cable connection only)</li><hr></font></body></html> "
};


char upgrade200_1[]={
	/* "<html><head><title>Simple HTTP Web Server</title></head>" */
	"<script language=\"javascript\"> var count=TAG:NUM1"
	"; function clock() {	var min = Math.floor(count / 60); 	var sec = count - min*60 ;"\
	"	document.form.mm.value= min; 	document.form.ss.value= sec;"\
	"	count--; 	if (count) 		setTimeout(\"clock()\",1000); "\
	"else setTimeout(\"location='/reboot.html'\",1000);  }"\
	"</script>"\
	"<noscript>"\
	"<meta http-equiv=\"Refresh\" content=\"5;URL=/reboot.html\" />"\
	"</noscript>"\
	"<body onLoad=\"clock();\">"\
	"	<center><font color=blue face=verdana size=3><b>Firmware Upgrade</b></font>"\
	"<form name=\"form\" method=\"post\" action="">"\
	"Please wait  <input type=\"text\" name=\"mm\" size=\"2\"> Min &nbsp;<input type=\"text\" name=\"ss\" size=\"2\"> Sec. &nbsp;"\
	"</Form>	"\
	"<hr><ul><font face=Verdana color=red size=4>NOTICE !!</font></ul><br></center>"\
	"<font face=Verdana size=1><li>Do not remove power and the LAN cable connected "
	"during the upgrading process. (Upgrade by cable connection only)</li><hr></font></body></html> "
};

char reboot200[]={
	/* "<html><head><title>Simple HTTP Web Server</title></head>" */
	"<body> <form> <center>"\
	"<font color=blue face=verdana size=3><b>Firmware Upgrade</b></font>"\
	"<p><p>"\
	"<font color=red face=verdana size=5><b><marquee width=\"400\" height=\"20\" behavior=alternate>Reboot System Now .....</marquee></b></font>"\
	"<p> <p> </form> "\
	"<hr><ul><font face=Verdana color=red size=4>NOTICE !!</font></ul><br></center>"\
	"<font face=Verdana size=1><li>Do not remove power and the LAN cable connected "
	"during the upgrading process. (Upgrade by cable connection only)</li><hr></font></body></html> "
};



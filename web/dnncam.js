  
/***********************************************
* Dynamic Ajax Content- © Dynamic Drive DHTML code library (www.dynamicdrive.com)
* This notice MUST stay intact for legal use
* Visit Dynamic Drive at http://www.dynamicdrive.com/ for full source code
***********************************************/
     
var bustcachevar=0 //bust potential caching of external pages after initial request? (1=yes, 0=no)
var loadedobjects=""
var rootdomain="http://"+window.location.hostname
var bustcacheparameter=""
     
function ajaxpage(url, containerid)
{
    var page_request = false;
    if (window.XMLHttpRequest) // if Mozilla, Safari etc
        page_request = new XMLHttpRequest();
    else if (window.ActiveXObject){ // if IE
        try {
            page_request = new ActiveXObject("Msxml2.XMLHTTP");
        } 
        catch (e){
            try{
                page_request = new ActiveXObject("Microsoft.XMLHTTP");
            }
            catch (e){}
        }
    }
    else
        return false;
    page_request.onreadystatechange=function(){
        loadpage(page_request, containerid)
    }
    if (bustcachevar) //if bust caching of external page
        bustcacheparameter=(url.indexOf("?")!=-1)? "&"+new Date().getTime() : "?"+new Date().getTime();
    page_request.open('GET', url+bustcacheparameter, true);
    page_request.send(null)
}
     
function loadpage(page_request, containerid){
    if (page_request.readyState == 4 && (page_request.status==200 || window.location.href.indexOf("http")==-1))
    {
        if(page_request.responseText == "")
            document.getElementById(containerid).innerHTML="Loading from server...";
        else
            document.getElementById(containerid).innerHTML=page_request.responseText;
    }
}

function loadobjs()
{
    if (!document.getElementById)
    {
        return;
    }

    for (i=0; i<arguments.length; i++)
    {
        var file=arguments[i]
            var fileref=""
            if (loadedobjects.indexOf(file)==-1)
            { //Check to see if this object has not already been added to page before proceeding
                if (file.indexOf(".js")!=-1){ //If object is a js file
                    fileref=document.createElement('script')
                    fileref.setAttribute("type","text/javascript");
                    fileref.setAttribute("src", file);
                }
                else if (file.indexOf(".css")!=-1){ //If object is a css file
                    fileref=document.createElement("link")
                    fileref.setAttribute("rel", "stylesheet");
                    fileref.setAttribute("type", "text/css");
                    fileref.setAttribute("href", file);
                }
            }
        if (fileref!=""){
            document.getElementsByTagName("head").item(0).appendChild(fileref);
            loadedobjects+=file+" ";//Remember this object as being already added to page
        }
    }
}

function getPageAsString(url)
{
    oxmlhttp = null;
    try
    {
        oxmlhttp = new XMLHttpRequest();
        oxmlhttp.overrideMimeType("text/xml");
    }
    catch(e)
    {
        try
        {
            oxmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
        }
        catch(e) { return null; }
    }
    if(!oxmlhttp) return null;
    try
    {
        oxmlhttp.open("GET",url,false);
        oxmlhttp.send(null);
    }
    catch(e) { return null; }
    return oxmlhttp.responseText;
}
                              

var set_buttons = function()
{
    status = getPageAsString('/cgi-bin/rpc?request=system_status');
}

var get_config = function()
{
    /* alert("get_config"); */
    ajaxpage('/cgi-bin/rpc?request=get_config', 'cam_config');
    //ajaxpage('/cgi-bin/temps', 'temps');
    //ajaxpage('/cgi-bin/rpc?request=get_timing', 'timing_data');
}

var set_auto_exposure = function()
{
    var val = document.getElementById("ae_lock_select").value
    if(val == "Off")
        val = "false"
    if(val == "On")
        val = "true"
    ajaxpage('/cgi-bin/rpc?request=set_auto_exposure+b/'+val, 'result');
}

var set_awb = function()
{
    var val = document.getElementById("awb_select").value
    if(val == "Off")
        val = "false"
    if(val == "On")
        val = "true"
    ajaxpage('/cgi-bin/rpc?request=set_awb+b/'+val, 'result');
}

var set_awb_mode = function()
{
    var val = document.getElementById("awb_mode_select").value
    ajaxpage('/cgi-bin/rpc?request=set_awb_mode+s/'+val, 'result');
}

var set_wb_gains = function()
{
    var val0 = document.getElementById("wb_gain_0").value
    var val1 = document.getElementById("wb_gain_1").value
    var val2 = document.getElementById("wb_gain_2").value
    var val3 = document.getElementById("wb_gain_3").value
    ajaxpage('/cgi-bin/rpc?request=set_awb_gains+d/' + val0 + '+d/' + val1 + '+d/' + val2 + '+d/' + val3, 'result');
}

var set_denoise_mode = function()
{
    var val = document.getElementById("denoise_mode_select").value
    ajaxpage('/cgi-bin/rpc?request=set_denoise_mode+s/'+val, 'result');
}

var set_stream_selection = function(val)
{
  //alert("set stream selection: "+val);
  ajaxpage('/cgi-bin/rpc?request=set_stream_selection+s/'+val, 'result');
}

var set_exposure = function()
{
    var value_low = document.getElementById("exposure_text_low").value
    value_low = Number.parseInt(value_low)
    var value_high = document.getElementById("exposure_text_high").value
    value_high = Number.parseInt(value_high)
    if(Number.isNaN(value_low))
        value_low = 16000
    if(Number.isNaN(value_high))
        value_high = 165770000
    if(value_high < value_low)
        value_high = value_low
    if(value_low < 16000)
        value_low = 16000
    if(value_low > 165770000)
        value_low = 165770000
    if(value_high < 16000)
        value_high = 16000
    if(value_high > 165770000)
        value_high = 165770000
    document.getElementById("exposure_text_low").value = value_low
    document.getElementById("exposure_text_high").value = value_high
    ajaxpage('/cgi-bin/rpc?request=set_exposure_time+I/'+value_low+'+I/'+value_high, 'result');
    get_config();
}

var set_frame_duration = function()
{
    var value_low = document.getElementById("frame_duration_text_low").value
    value_low = Number.parseInt(value_low)
    var value_high = document.getElementById("frame_duration_text_high").value
    value_high = Number.parseInt(value_high)
    if(Number.isNaN(value_low))
        value_low = 16666667
    if(Number.isNaN(value_high))
        value_high = 1462525056
    if(value_high < value_low)
        value_high = value_low
    if(value_low < 16666667)
        value_low = 16666667
    if(value_low > 1462525056)
        value_low = 1462525056
    if(value_high < 16666667)
        value_high = 16666667
    if(value_high > 1462525056)
        value_high = 1462525056
    document.getElementById("frame_duration_text_low").value = value_low
    document.getElementById("frame_duration_text_high").value = value_high
    ajaxpage('/cgi-bin/rpc?request=set_frame_duration+I/'+value_low+'+I/'+value_high, 'result');
    get_config();
}

var set_focus_relative = function(val)
{
    var value = document.getElementsByName("focus_relative")[0].value
    value = Number.parseInt(value)
    if(Number.isNaN(value))
        value = 0
    if(value < 0)
        value = 0
    if(value > 7000)
        value = 7000
    document.getElementsByName("focus_relative")[0].value = value
    if(value == 0)
        return
    ajaxpage('/cgi-bin/rpc?request=focus_relative+i/'+value*val, 'result');
    get_config();
}

var set_zoom_relative = function(val)
{
    var value = document.getElementsByName("zoom_relative")[0].value
    value = Number.parseInt(value)
    if(Number.isNaN(value))
        value = 0
    if(value < 0)
        value = 0
    if(value > 7000)
        value = 7000
    document.getElementsByName("zoom_relative")[0].value = value
    if(value == 0)
        return
    ajaxpage('/cgi-bin/rpc?request=zoom_relative+i/'+value*val, 'result');
    get_config();
}

var set_iris_relative = function(val)
{
    var value = document.getElementsByName("iris_relative")[0].value
    value = Number.parseInt(value)
    if(Number.isNaN(value))
        value = 0
    if(value < 0)
        value = 0
    if(value > 120)
        value = 120
    document.getElementsByName("iris_relative")[0].value = value
    if(value == 0)
        return
    ajaxpage('/cgi-bin/rpc?request=iris_relative+i/'+value*val, 'result');
    get_config();
}

var set_gain = function()
{
    var value_low = document.getElementById("gain_text_low").value
    value_low = Number.parseInt(value_low)
    var value_high = document.getElementById("gain_text_high").value
    value_high = Number.parseInt(value_high)
    if(Number.isNaN(value_low))
        value_low = 1
    if(Number.isNaN(value_high))
        value_high = 180
    if(value_high < value_low)
        value_high = value_low
    if(value_low < 1)
        value_low = 1
    if(value_low > 180)
        value_low = 180
    if(value_high < 1)
        value_high = 1
    if(value_high > 180)
        value_high = 180
    document.getElementById("gain_text_low").value = value_low
    document.getElementById("gain_text_high").value = value_high
    ajaxpage('/cgi-bin/rpc?request=set_gain+I/'+value_low+'+I/'+value_high, 'result');
    get_config();
}

var set_denoise_strength = function()
{
    var value = document.getElementById("denoise_strength_text").value
    value = Number.parseFloat(value)
    if(Number.isNaN(value))
        return
    if(value < -1)
        value = -1
    if(value > 1)
        value = 1
    document.getElementById("denoise_strength_text").value = value
    ajaxpage('/cgi-bin/rpc?request=set_denoise_strength+d/'+value, 'result');
    get_config();
}

var refresh = function()
{
    get_config();
    //set_buttons();
    setTimeout("refresh();",3000);
}

if (document.addEventListener) {
  document.addEventListener("DOMContentLoaded", refresh, false);
}

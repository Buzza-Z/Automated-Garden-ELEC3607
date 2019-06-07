$(document).ready(function(){
    fetchpoll();
});
function halt(chan) {
    let cmd_str = `AH${chan}`;
    
    jQuery.get("int", {cmd: cmd_str});
}
function arm(chan) {
    let cmd_str = `AA${chan}`;
    
    jQuery.get("int", {cmd: cmd_str});
}
function submitCmd() {
    let nchan = document.getElementById("nchan").value;
    let nmode = document.getElementById("nmode").value;
    let nfreq = document.getElementById("nfreq").value;
    let ngoal = document.getElementById("ngoal").value;
    
    let cmd_str = `S${nchan},${nmode},${nfreq},${ngoal}`;
    
    jQuery.get("int", {cmd: cmd_str});
    
}

function setText(id,newvalue) {
  var s = document.getElementById(id);
  s.innerHTML = newvalue;
}

function setChannelSetText(chan, data) {
    let goal_unit = "";
    $.each( data, function( key, val ) {
        switch(key) {
          case "mode":
            let mode_fname = "";
            switch(val) {
              case 0:
                mode_fname = "Manual";
                break;
              case 1:
                mode_fname = "Timed";
                goal_unit = "ms";
                break;
              case 2:
                mode_fname = "Watered";
                goal_unit = "L";
                break;
              default:
                break;
            }
            setText("m"+chan, mode_fname);
            break;
          case "frequency":
            setText("f"+chan, val+"ms");
            break;
          case "goal":
            setText("g"+chan, val+goal_unit);
            break;          
        case "status":
            setText("st"+chan, val ? "On" : "Off");
            break;
          default:
            break;
        }
    });
}
function fetchpoll(){
    $.getJSON( "poll", function( data ) {
      $.each( data, function( key, val ) {
            switch(key) {
              case "temp":
                setText("temp", val);
                break;
              case "moisture":
                setText("moisture", val);
                break;
              case "water":
                setText("wtr", val);
                break;
              case "ch1":
              case "ch2":
              case "ch3":
              case "ch4":
                setChannelSetText(key.substring(2), val);
                break;
              default:
                // code block
                break;
            }
        });
    });
    setTimeout(fetchpoll, 1000);
}
return [===[<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>Leda HTTP Controller: Real-time updates</title>
	<link href="leda.css" rel="stylesheet" type="text/css">
	<script language="javascript" type="text/javascript" src="jquery.js"></script>
	<script language="javascript" type="text/javascript" src="jquery.flot.js"></script>
	<script language="javascript" type="text/javascript" src="arbor.js"></script>
	<script language="javascript" type="text/javascript" src="graphics.js"></script>
	<script language="javascript" type="text/javascript" src="jquery.flot.navigate.js"></script>
	<script type="text/javascript">

	var clear_pan=function() {
	plotStageLatency = $.plot("#stage_latency", stagesLatency, {
			series: {
				shadowSize: 1	// Drawing is faster without shadows
			},
			zoom: {
				interactive: true
			},
			pan: {
				interactive: true
			}
	});

	plotStageThroughput = $.plot("#stage_throughput", stagesThrouhput, {
			series: {
				shadowSize: 0
			},
			zoom: {
				interactive: true
			},
			pan: {
				interactive: true
			}
	});

	plotStageQueue = $.plot("#stage_queue", stagesQueue, {
			series: {
				shadowSize: 0
			},
			zoom: {
				interactive: true
			},
			pan: {
				interactive: true
			}
	});

	plotStageInstances = $.plot("#stage_instances", stagesInstances, {
			series: {
				shadowSize: 0
			},
			zoom: {
				interactive: true
			},
			pan: {
				interactive: true
			}
	});

	plotConnectorLatency = $.plot("#connectors_latency", connectorsLatency, {
			series: {
				shadowSize: 0
			},
			zoom: {
				interactive: true
			},
			pan: {
				interactive: true
			}
	});
	
	plotConnectorThroughput = $.plot("#connectors_throughput", connectorsThroughput, {
			series: {
				shadowSize: 0
			},
			zoom: {
				interactive: true
			},
			pan: {
				interactive: true
			}
	});
	}

	var clear_graphs=function() {
	latencyData={}
	throughputData={}
	queueData={}
	instancesData={}

	stagesLatency=[]
	stagesThrouhput=[]
	stagesQueue=[]
	stagesInstances=[]

	latencyCData={}
	throughputCData={}

	connectorsLatency=[]
	connectorsThroughput=[]


	colors={}

	var i=0

	info.stages.forEach(function(entry) {
   	latencyData[entry.name]=[[info.uptime,entry.latency]]
   	throughputData[entry.name]=[[info.uptime,entry.throughput]]
   	queueData[entry.name]=[[info.uptime,entry.queue]]
   	instancesData[entry.name]=[[info.uptime,entry.ready]]
		stagesLatency.push({label: entry.name,data: latencyData[entry.name]})
		stagesThrouhput.push({label: entry.name,data: throughputData[entry.name]})
		stagesQueue.push({label: entry.name,data: queueData[entry.name]})
		stagesInstances.push({label: entry.name,data: instancesData[entry.name]})
		colors[entry.name]=i++;
	});
	
	i=0
	
	info.connectors.forEach(function(entry) {
   	latencyCData[entry.name]=[[info.uptime,entry.latency]]
   	throughputCData[entry.name]=[[info.uptime,entry.throughput]]
		connectorsLatency.push({label: entry.name,data: latencyCData[entry.name]})
		connectorsThroughput.push({label: entry.name,data: throughputCData[entry.name]})
		colors[entry.name]=i++;
	});
	}

	var main=function () {
	var updateInterval = 1000;

	$("#updateInterval").val(updateInterval).change(function () {
			var v = $(this).val();
			if (v && !isNaN(+v)) {
				updateInterval = +v;
				if (updateInterval < 1) {
					updateInterval = 1;
				} else if (updateInterval > 2000) {
					updateInterval = 2000;
				}
				$(this).val("" + updateInterval);
			}
		});

	function getInfo() {
		var stage={}

		function onDataReceived(series) {
       stage=series
       $("#app-name")[0].innerHTML=series.name
   	}
   	function onError(error,i,str) {
			alert('ERROR: '+str)
   	}

		$.ajax({
			url: "stats",
			type: "GET",
			dataType: "json",
			success: onDataReceived,
			error: onError,
			async: false
		});
		return stage
	}
	info=getInfo()
	clear_graphs();
	clear_pan();
	



	// insert checkboxes 
	var choiceContainer = $("#stage_choices");

	$.each(stagesLatency, function(key, val) {
		choiceContainer.append("&nbsp;<input type='checkbox' name='" + key +
			"' checked='checked' value='" + val.label +"' id='stage_id" + key + "' onClick='drawGraphs();'></input>" +
			"<label for='id" + key + "'>"
			+ val.label + "</label>");
	});	

	var choiceContainerC = $("#connector_choices");
	$.each(connectorsLatency, function(key, val) {
		choiceContainerC.append("&nbsp;<input type='checkbox' name='" + key +
			"' checked='checked' value='" + val.label +"' id='connector_id" + key + "' onClick='drawGraphs();'></input>" +
			"<label for='id" + key + "'>"
			+ val.label + "</label>");
	});	
	
		drawGraphs=function () {
					var dataLatency=[]
            	var dataThroughput=[]
            	var dataQueue=[]
            	var dataInstances=[]
            	
            	var dataCLatency=[]
            	var dataCThroughput=[]
            	
  					choiceContainer.find("input:checked").each(function () {
						var key = $(this).attr("name");
						var name = $(this).attr("value");
	   	         dataLatency.push({label: name,data: latencyData[name], color:colors[name]})
	   	         dataThroughput.push({label: name,data: throughputData[name], color:colors[name]})
	   	         dataQueue.push({label: name,data: queueData[name], color:colors[name]})
	   	         dataInstances.push({label: name,data: instancesData[name], color:colors[name]})
					});
					
					choiceContainerC.find("input:checked").each(function () {
						var key = $(this).attr("name");
						var name = $(this).attr("value");
	   	         dataCLatency.push({label: name,data: latencyCData[name], color:colors[name]})
	   	         dataCThroughput.push({label: name,data: throughputCData[name], color:colors[name]})
					});
					
					
					plotStageLatency.setData(dataLatency);
					plotStageThroughput.setData(dataThroughput);
					plotStageQueue.setData(dataQueue);
					plotStageInstances.setData(dataInstances);	
					plotConnectorLatency.setData(dataCLatency);
					plotConnectorThroughput.setData(dataCThroughput);

					plotStageLatency.setupGrid()
					plotStageLatency.draw();

					plotStageThroughput.setupGrid()
					plotStageThroughput.draw();

					plotStageQueue.setupGrid()
					plotStageQueue.draw();

					plotStageInstances.setupGrid()
					plotStageInstances.draw();
					
					plotConnectorLatency.setupGrid()
					plotConnectorLatency.draw();

					plotConnectorThroughput.setupGrid()
					plotConnectorThroughput.draw();
	}

	var update=0;

	var updateData=function () {	
            function onDataReceived(series) {
					info.uptime=series.uptime
					$("#threads-text")[0].innerHTML=series.active_threads+"/"+series.thread_pool_size
					$("#memory-usage")[0].innerHTML=(series.mem.Rss/1024).toFixed(2)+"MB / "+(series.mem.total/(1024*1024)).toFixed(2)+"GB ("+series.mem.percentage.toFixed(2)+"%)"
   	         series.stages.forEach(function(entry) {
						latencyData[entry.name].push([series.uptime,entry.latency])
	   	         throughputData[entry.name].push([series.uptime,entry.throughput])
	   	         queueData[entry.name].push([series.uptime,entry.queue])
	   	         instancesData[entry.name].push([series.uptime,entry.ready])

					});
					
   	         series.connectors.forEach(function(entry) {
   	         	latencyCData[entry.name].push([series.uptime,entry.latency])
	   	         throughputCData[entry.name].push([series.uptime,entry.throughput])
					});

					
					drawGraphs()
					setTimeout(updateData, updateInterval);
            }

            function onDataError(error,i,str) {
					$("#status-text")[0].innerHTML="Offline"
					$("#status-text")[0].style.color="#770000"
            }

				$.ajax({
					url: "stats",
					type: "GET",
					dataType: "json",
					success: onDataReceived,
					error: onDataError
				});
	}
	
	increase_threads=function () {	
            function onDataReceived(series) {
            }

            function onDataError(error,i,str) {
					$("#status-text")[0].innerHTML="Offline"
					$("#status-text")[0].style.color="#770000"
            }

				$.ajax({
					url: "increase_threads",
					type: "POST",
					data: "increment=1",
					success: onDataReceived,
					error: onDataError
				});
	}

	trim_memory=function () {	
            function onDataReceived(series) {
            }

            function onDataError(error,i,str) {
					$("#status-text")[0].innerHTML="Offline"
					$("#status-text")[0].style.color="#770000"
            }

				$.ajax({
					url: "trim",
					type: "GET",
					success: onDataReceived,
					error: onDataError
				});
	}

	inc_threads=function () {	
            function onDataReceived(series) {
            }

            function onDataError(error,i,str) {
					$("#status-text")[0].innerHTML="Offline"
					$("#status-text")[0].style.color="#770000"
            }

				$.ajax({
					url: "increase_threads",
					type: "GET",
					success: onDataReceived,
					error: onDataError
				});
	}
	dec_threads=function () {	
            function onDataReceived(series) {
            }

            function onDataError(error,i,str) {
					$("#status-text")[0].innerHTML="Offline"
					$("#status-text")[0].style.color="#770000"
            }

				$.ajax({
					url: "decrease_threads",
					type: "GET",
					success: onDataReceived,
					error: onDataError
				});
	}


	//GRAPH

/*
 Renderer = function(canvas){
    var canvas = $(canvas).get(0)
    var ctx = canvas.getContext("2d");
    var gfx = arbor.Graphics(canvas)
    var particleSystem = null

    var that = {
      init:function(system){
        particleSystem = system
        particleSystem.screenSize(canvas.width, canvas.height) 
        particleSystem.screenPadding(40)

        that.initMouseHandling()
      },

      redraw:function(){
        if (!particleSystem) return

        gfx.clear() // convenience ƒ: clears the whole canvas rect

        // draw the nodes & save their bounds for edge drawing
        var nodeBoxes = {}
        particleSystem.eachNode(function(node, pt){
          // node: {mass:#, p:{x,y}, name:"", data:{}}
          // pt:   {x:#, y:#}  node position in screen coords

          // determine the box size and round off the coords if we'll be 
          // drawing a text label (awful alignment jitter otherwise...)
          var label = node.data.label||""
          var w = ctx.measureText(""+label).width + 10
          if (!(""+label).match(/^[ \t]*$/)){
            pt.x = Math.floor(pt.x)
            pt.y = Math.floor(pt.y)
          }else{
            label = null
          }

          // draw a rectangle centered at pt
          if (node.data.color) ctx.fillStyle = node.data.color
          else ctx.fillStyle = "rgba(0,0,0,.2)"
          if (node.data.color=='none') ctx.fillStyle = "white"

          if (node.data.shape=='dot'){
            gfx.oval(pt.x-w/2, pt.y-w/2, w,w, {fill:ctx.fillStyle})
            nodeBoxes[node.name] = [pt.x-w/2, pt.y-w/2, w,w]
          }else{
            gfx.rect(pt.x-w/2, pt.y-10, w,20, 4, {fill:ctx.fillStyle})
            nodeBoxes[node.name] = [pt.x-w/2, pt.y-11, w, 22]
          }

          // draw the text
          if (label){
            ctx.font = "12px Helvetica"
            ctx.textAlign = "center"
            ctx.fillStyle = "white"
            if (node.data.color=='none') ctx.fillStyle = '#333333'
            ctx.fillText(label||"", pt.x, pt.y+4)
            ctx.fillText(label||"", pt.x, pt.y+4)
          }
        })    			


        // draw the edges
        particleSystem.eachEdge(function(edge, pt1, pt2){
          // edge: {source:Node, target:Node, length:#, data:{}}
          // pt1:  {x:#, y:#}  source position in screen coords
          // pt2:  {x:#, y:#}  target position in screen coords

          var weight = edge.data.weight
          var color = edge.data.color

          if (!color || (""+color).match(/^[ \t]*$/)) color = null

          // find the start point
          var tail = intersect_line_box(pt1, pt2, nodeBoxes[edge.source.name])
          var head = intersect_line_box(tail, pt2, nodeBoxes[edge.target.name])

          ctx.save() 
            ctx.beginPath()
            ctx.lineWidth = (!isNaN(weight)) ? parseFloat(weight) : 1
            ctx.strokeStyle = (color) ? color : "#cccccc"
            ctx.fillStyle = null

            ctx.moveTo(tail.x, tail.y)
            ctx.lineTo(head.x, head.y)
            ctx.stroke()
          ctx.restore()

          // draw an arrowhead if this is a -> style edge
          if (edge.data.directed){
            ctx.save()
              // move to the head position of the edge we just drew
              var wt = !isNaN(weight) ? parseFloat(weight) : 1
              var arrowLength = 6 + wt
              var arrowWidth = 2 + wt
              ctx.fillStyle = (color) ? color : "#cccccc"
              ctx.translate(head.x, head.y);
              ctx.rotate(Math.atan2(head.y - tail.y, head.x - tail.x));

              // delete some of the edge that's already there (so the point isn't hidden)
              ctx.clearRect(-arrowLength/2,-wt/2, arrowLength/2,wt)

              // draw the chevron
              ctx.beginPath();
              ctx.moveTo(-arrowLength, arrowWidth);
              ctx.lineTo(0, 0);
              ctx.lineTo(-arrowLength, -arrowWidth);
              ctx.lineTo(-arrowLength * 0.8, -0);
              ctx.closePath();
              ctx.fill();
            ctx.restore()
          }
        })



      },
      initMouseHandling:function(){
        // no-nonsense drag and drop (thanks springy.js)
        selected = null;
        nearest = null;
        var dragged = null;
        var oldmass = 1

        // set up a handler object that will initially listen for mousedowns then
        // for moves and mouseups while dragging
        var handler = {
          clicked:function(e){
            var pos = $(canvas).offset();
            _mouseP = arbor.Point(e.pageX-pos.left, e.pageY-pos.top)
            selected = nearest = dragged = particleSystem.nearest(_mouseP);

            if (dragged.node !== null) dragged.node.fixed = true

            $(canvas).bind('mousemove', handler.dragged)
            $(window).bind('mouseup', handler.dropped)

            return false
          },
          dragged:function(e){
            var old_nearest = nearest && nearest.node._id
            var pos = $(canvas).offset();
            var s = arbor.Point(e.pageX-pos.left, e.pageY-pos.top)

            if (!nearest) return
            if (dragged !== null && dragged.node !== null){
              var p = particleSystem.fromScreen(s)
              dragged.node.p = p
            }

            return false
          },

          dropped:function(e){
            if (dragged===null || dragged.node===undefined) return
            if (dragged.node !== null) dragged.node.fixed = false
            dragged.node.tempMass = 50
            dragged = null
            selected = null
            $(canvas).unbind('mousemove', handler.dragged)
            $(window).unbind('mouseup', handler.dropped)
            _mouseP = null
            return false
          }
        }
        $(canvas).mousedown(handler.clicked);

      }

    }

    // helpers for figuring out where to draw arrows (thanks springy.js)
    var intersect_line_line = function(p1, p2, p3, p4)
    {
      var denom = ((p4.y - p3.y)*(p2.x - p1.x) - (p4.x - p3.x)*(p2.y - p1.y));
      if (denom === 0) return false // lines are parallel
      var ua = ((p4.x - p3.x)*(p1.y - p3.y) - (p4.y - p3.y)*(p1.x - p3.x)) / denom;
      var ub = ((p2.x - p1.x)*(p1.y - p3.y) - (p2.y - p1.y)*(p1.x - p3.x)) / denom;

      if (ua < 0 || ua > 1 || ub < 0 || ub > 1)  return false
      return arbor.Point(p1.x + ua * (p2.x - p1.x), p1.y + ua * (p2.y - p1.y));
    }

    var intersect_line_box = function(p1, p2, boxTuple)
    {
      var p3 = {x:boxTuple[0], y:boxTuple[1]},
          w = boxTuple[2],
          h = boxTuple[3]

      var tl = {x: p3.x, y: p3.y};
      var tr = {x: p3.x + w, y: p3.y};
      var bl = {x: p3.x, y: p3.y + h};
      var br = {x: p3.x + w, y: p3.y + h};

      return intersect_line_line(p1, p2, tl, tr) ||
            intersect_line_line(p1, p2, tr, br) ||
            intersect_line_line(p1, p2, br, bl) ||
            intersect_line_line(p1, p2, bl, tl) ||
            false
    }

    return that
  } 

	var sys = arbor.ParticleSystem(1000, 400,1);
	sys.parameters({gravity:true});

	sys.renderer = Renderer("#gviewport") ;

var data = {
nodes:{
animals:{'color':'red','shape':'dot','label':'Animals'},
dog:{'color':'green','shape':'dot','label':'dog'},
cat:{'color':'blue','shape':'dot','label':'cat'}
},
edges:{
animals:{ dog:{}, cat:{} }
}
};
sys.graft(data);
*/
	updateData();
}
$(main); 
</script>
</head>
<body>

	<div id="header">
		<h2><img src='leda.svg' height='180' style="float:right"/>Leda Real-time Statistics</h2>
	</div>

	<div id="commands">
		<h2>Application: <span id="app-name"></span></h2>
		<p>Controller status: <span id="status-text" style="color: #007700;">Online</span></p>
		<p>Memory usage: <span id="memory-usage"></span> - <a href="#" onCLick="trim_memory(); return false;">Trim</a></p>
		<p>Active threads: <span id="threads-text"></span> - <a href="#" onCLick="inc_threads(); return false;">Increase</a> <a href="#" onCLick="dec_threads(); return false;">Decrease</a></p>
		<p>Graphs: <a href="#" onCLick="clear_graphs(); return false;">Clear data</a> <a href="#" onCLick="clear_pan(); return false;">Reset pan</a></p>
	</div>

	<!-- <canvas id="gviewport" width="800" height="600"></canvas> -->

	<div id="content">
		<p id='status'></p>
		<p>Time between updates: <input id="updateInterval" type="text" value="" style="text-align: right; width:5em"> milliseconds</p>
		
		<div id="stage_choices" class="choices"><h2>Stages:</h2></div>
		<div class="div-text-1">Stage Latency (s)</div>
		<div class="div-text-2">Stage's Queue size</div>
		<div class="div-text-3">Stage Throughput (ev/s)</div>
		<div class="div-text-4">Stage Instances</div>
		<div class="stages" id="stages-div">
			<div id="stage_latency" class="stage-graph"></div>
			<div id="stage_throughput" class="stage-graph-1"></div>
			<div id="stage_queue" class="stage-graph-2"></div>
			<div id="stage_instances" class="stage-graph-3"></div>
		</div> 
		<div id="connector_choices" class="choices"><h2>Connectors:</h2></div>
				<div class="div-text-1">Connector Latency (s)</div>
				<div class="div-text-2">Connector Throughput (ev/s)</div>
		<div class="connectors">
			<div id="connectors_latency" class="connector-graph"></div>
			<div id="connectors_throughput" class="connector-graph-2"></div>
		</div> 
	</div>

</body>
</html>
]===]

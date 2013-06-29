return [===[<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>Leda HTTP Controller: Real-time updates</title>
	<link href="leda.css" rel="stylesheet" type="text/css">
	<script language="javascript" type="text/javascript" src="jquery.js"></script>
	<script language="javascript" type="text/javascript" src="jquery.flot.js"></script>
	<script language="javascript" type="text/javascript" src="jquery.arbor.js"></script>
	<script language="javascript" type="text/javascript" src="jquery.flot.navigate.js"></script>
	<script type="text/javascript">

	$(function () {
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

	setText=function(series) {

	}
	
	function getInfo() {
		var stage={}

		function onDataReceived(series) {
       stage=series
       setText(series)
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

	var info=getInfo()
	latencyData={}
	throughputData={}
	queueData={}
	instancesData={}

	var stagesLatency=[]
	var stagesThrouhput=[]
	var stagesQueue=[]
	var stagesInstances=[]

	latencyCData={}
	throughputCData={}

	var connectorsLatency=[]
	var connectorsThroughput=[]


	var colors={}

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



	// insert checkboxes 
	var choiceContainer = $("#stage_choices");
	$.each(stagesLatency, function(key, val) {
		choiceContainer.append("<br/><input type='checkbox' name='" + key +
			"' checked='checked' value='" + val.label +"' id='stage_id" + key + "' onClick='drawGraphs();'></input>" +
			"<label for='id" + key + "'>"
			+ val.label + "</label>");
	});	

	var choiceContainerC = $("#connector_choices");
	$.each(connectorsLatency, function(key, val) {
		choiceContainerC.append("<br/><input type='checkbox' name='" + key +
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
				   setText(series)
					setTimeout(updateData, updateInterval);
            }

            function onDataError(error,i,str) {
					alert('ERROR: '+str)
            }

				$.ajax({
					url: "stats",
					type: "GET",
					dataType: "json",
					success: onDataReceived,
					error: onDataError
				});
	}

	updateData();

	
});
</script>
</head>
<body>

	<div id="header">
		<h2>Real-time updates</h2>
	</div>

	<div id="content">
		<p id='status'></p>
		<p>Time between updates: <input id="updateInterval" type="text" value="" style="text-align: right; width:5em"> milliseconds</p>
		Stages:
		<div id="stage_choices" class="choices">Stages</div>
				<div class="div-text-1">Latency</div>
				<div class="div-text-2">Queue size</div>
				<div class="div-text-3">Throughput</div>
				<div class="div-text-4">Instances</div>
		<div class="stages">
			<div id="stage_latency" class="stage-graph"></div>
			<div id="stage_throughput" class="stage-graph-1"></div>
			<div id="stage_queue" class="stage-graph-2"></div>
			<div id="stage_instances" class="stage-graph-3"></div>
		</div> 
		Connectors:
		<div id="connector_choices" class="choices">Connectors</div>
				<div class="div-text-1">Latency</div>
				<div class="div-text-2">Throughput</div>
		<div class="connectors">
			<div id="connectors_latency" class="connector-graph"></div>
			<div id="connectors_throughput" class="connector-graph-2"></div>
		</div> 
	</div>

</body>
</html>
]===]

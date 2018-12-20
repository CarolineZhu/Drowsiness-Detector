$(document).ready(function () {
  var timeData = [],
    bpmData = [],
    hrvData = [];
  var data = {
    labels: timeData,
    datasets: [
      {
        fill: false,
        label: 'BPM',
        yAxisID: 'BPM',
        borderColor: "rgba(72, 211, 224, 1)",
        pointBoarderColor: "rgba(72, 211, 224, 1)",
        backgroundColor: "rgba(72, 211, 224, 0.4)",
        pointHoverBackgroundColor: "rgba(72, 211, 224, 1)",
        pointHoverBorderColor: "rgba(72, 211, 224, 1)",
        data: bpmData
      },
      {
        fill: false,
        label: 'HRV',
        yAxisID: 'HRV',
        borderColor: "rgba(255, 111, 97, 1)",
        pointBoarderColor: "rgba(255, 111, 97, 1)",
        backgroundColor: "rgba(255, 111, 97, 0.4)",
        pointHoverBackgroundColor: "rgba(255, 111, 97, 1)",
        pointHoverBorderColor: "rgba(255, 111, 97, 1)",
        data: hrvData
      }
    ]
  }

  var basicOption = {
    title: {
      display: true,
      text: 'BPM & HRV Real-time Data',
      fontSize: 36
    },
    scales: {
      yAxes: [{
        id: 'BPM',
        type: 'linear',
        scaleLabel: {
          labelString: 'Beats per Min',
          display: true
        },
        position: 'left',
      }, {
          id: 'HRV',
          type: 'linear',
          scaleLabel: {
            labelString: 'Heart Rate Variability',
            display: true
          },
          position: 'right'
        }]
    }
  }

  //Get the context of the canvas element we want to select
  var ctx = document.getElementById("myChart").getContext("2d");
  var optionsNoAnimation = { animation: false }
  var myLineChart = new Chart(ctx, {
    type: 'line',
    data: data,
    options: basicOption
  });

  var ws = new WebSocket('wss://' + location.host);
  ws.onopen = function () {
    console.log('Successfully connect WebSocket');
  }
  ws.onmessage = function (message) {
    console.log('receive message' + message.data);
    try {
      var obj = JSON.parse(message.data);
      if(!obj.time || !obj.bpm) {
        return;
      }
      timeData.push(obj.time);
      bpmData.push(obj.bpm);
      // only keep no more than 600 points in the line chart
      const maxLen = 600;
      var len = timeData.length;
      if (len > maxLen) {
        timeData.shift();
        bpmData.shift();
      }

      if (obj.hrv) {
        hrvData.push(obj.hrv);
      }
      if (hrvData.length > maxLen) {
        hrvData.shift();
      }

      myLineChart.update();
    } catch (err) {
      console.error(err);
    }
  }
});

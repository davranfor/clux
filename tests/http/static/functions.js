let startTime, chronoTimeout;
let isRunning = false;

function formatTime(ms) {
    const totalSeconds = Math.floor(ms / 1000);
    return [
        Math.floor(totalSeconds / 3600).toString().padStart(2, '0'),
        Math.floor((totalSeconds % 3600) / 60).toString().padStart(2, '0'),
        (totalSeconds % 60).toString().padStart(2, '0')
    ].join(':');
}

function updateChrono() {
    clockingTimer.textContent = formatTime(Date.now() - startTime);
    const msToNextSecond = 1000 - (Date.now() % 1000);
    chronoTimeout = setTimeout(updateChrono, msToNextSecond);
}

function startChrono(time = null) {
    if (chronoTimeout) clearTimeout(chronoTimeout);

    if (time) {
        const dateParts = time.split(/[- :]/);
        const dateObj = new Date(
            parseInt(dateParts[0]),     // year
            parseInt(dateParts[1]) - 1, // month (0-11)
            parseInt(dateParts[2]),     // day
            parseInt(dateParts[3]),     // hours
            parseInt(dateParts[4]),     // minutes
            parseInt(dateParts[5])      // seconds
        );

        startTime = dateObj.getTime();
    } else {
        startTime = Date.now();
    }
    clockingTimer.textContent = '00:00:00';
    isRunning = true;
    updateChrono();
}

function stopChrono() {
    if (isRunning) {
        clearTimeout(chronoTimeout);
        clockingTimer.textContent = '';
        isRunning = false;
    }
}

function shortDateTime(datetime) {
  const [date, time] = datetime.split(' ');
  const [y, m, d] = date.split('-');
  const [h, M, s] = time.split(':');
  
  return `${d}/${m}/${y} ${h}:${M}`;
}

function longDateTime(datetime) {
  const [date, time] = datetime.split(' ');
  const [y, m, d] = date.split('-');
  
  return `${d}/${m}/${y} ${time}`;
}
function isMobileDevice()
{
    return /Mobi|Android|iPhone|iPad|iPod/i.test(navigator.userAgent)
        && window.innerWidth <= 768;
}


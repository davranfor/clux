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

function showModal(message = "Sistema de fichaje | IGASA") {
    if (message.length > 512) {
        message = message.substring(0, 509) + '...';
    }

    const overlay = document.createElement('div');

    overlay.style.position = 'fixed';
    overlay.style.top = '0';
    overlay.style.left = '0';
    overlay.style.width = '100%';
    overlay.style.height = '100%';
    overlay.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
    overlay.style.zIndex = '1000';
    overlay.style.display = 'flex';
    overlay.style.justifyContent = 'center';
    overlay.style.alignItems = 'center';

    const modal = document.createElement('div');

    modal.style.borderRadius = '8px';
    modal.style.boxShadow = '0 4px 8px rgba(0, 0, 0, 0.2)';
    modal.style.backgroundColor = '#2d3748';
    modal.style.maxWidth = '400px';
    modal.style.width = '80%';
    modal.style.textAlign = 'center';
    modal.style.padding = '20px';

    const text = document.createElement('p');

    text.style.marginBottom = '20px';
    text.style.color = '#fffff';
    text.textContent = message;

    const button = document.createElement('button');

    button.style.border = 'none';
    button.style.borderRadius = '4px';
    button.style.backgroundColor = '#4caf50';
    button.style.color = '#ffffff';
    button.style.fontWeight = 'bold';
    button.style.padding = '10px 20px';
    button.style.cursor = 'pointer';
    button.textContent = 'Aceptar';

    button.addEventListener('click', function() {
        document.body.removeChild(overlay);
        document.body.style.overflow = '';
    });

    modal.appendChild(text);
    modal.appendChild(button);
    overlay.appendChild(modal);

    document.body.style.overflow = 'hidden';
    document.body.appendChild(overlay);

    button.focus();
}


function formatDate(date) {
  const d = String(date.getDate()).padStart(2, '0');
  const m = String(date.getMonth() + 1).padStart(2, '0');
  const y = date.getFullYear();
  return `${d}/${m}/${y}`;
}

function formatTime(ms) {
  const totalSeconds = Math.floor(ms / 1000);

  return [
    Math.floor(totalSeconds / 3600).toString().padStart(2, '0'),
    Math.floor((totalSeconds % 3600) / 60).toString().padStart(2, '0'),
    (totalSeconds % 60).toString().padStart(2, '0')
  ].join(':');
}

function timeDiff(a, b) {
  const diffMs = a - b;
  const diffMin = Math.abs(Math.floor(diffMs / 60000)); // 1 min = 60000 ms
  const h = Math.floor(diffMin / 60);
  const m = diffMin % 60;
  const sign = diffMs < 0 ? '-' : '';

  return `${sign}${String(h)}h ${String(m).padStart(2, '0')}m`;
}

function dayOfWeek(datetime) {
  const days = ['Domingo', 'Lunes', 'Martes', 'Miércoles', 'Jueves', 'Viernes', 'Sábado'];

  return days[datetime.getDay()];
}

function longDate(datetime) {
  const [y, m, d] = datetime.split(' ')[0].split('-');

  return `${d}/${m}/${y}`;
}

function shortTime(datetime) {
  const [h, m, s] = datetime.split(' ')[1].split(':');

  return `${h}:${m}`;
}

function longTime(datetime) {
  const [h, m, s] = datetime.split(' ')[1].split(':');

  return `${h}:${m}:${s}`;
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

function pairDateTime(datetime) {
  const [date, time] = datetime.split(' ');
  const [h, M, s] = time.split(':');

  return [date, `${h}:${M}`];
}

function formHash(form) {
  let h1 = 0xDEADBEEF, h2 = 0x41C6CE57;
  
  form.querySelectorAll('input, select, textarea').forEach(el => {
    const val = el.value || '';

    for (let i = 0; i < val.length; i++) {
      const c = val.charCodeAt(i);

      h1 = Math.imul(h1 ^ c, 0x85EBCA77);
      h2 = Math.imul(h2 ^ c, 0xC2B2AE3D);
    }
  });
  return (h1 ^ h2) >>> 0;
}

function isMobileDevice()
{
  return /Mobi|Android|iPhone|iPad|iPod/i.test(navigator.userAgent)
    && window.innerWidth <= 768;
}

function showMessage(message = "") {
  return new Promise((resolve) => {
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
    text.style.color = '#ffffff';
    text.textContent = message;

    const button = document.createElement('button');

    button.style.backgroundColor = '#4caf50';
    button.style.color = '#ffffff';
    button.style.outline = 'none';
    button.style.padding = '10px 20px';
    button.textContent = 'Aceptar';

    button.addEventListener('click', function() {
      document.body.removeChild(overlay);
      document.body.style.overflow = '';
      resolve();
    });

    modal.appendChild(text);
    modal.appendChild(button);
    overlay.appendChild(modal);

    document.body.style.overflow = 'hidden';
    document.body.appendChild(overlay);

    button.focus();
  });
}

function confirmMessage(message = "") {
  return new Promise((resolve) => {
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
    text.style.color = '#ffffff';
    text.textContent = message;

    const buttonsContainer = document.createElement('div');

    buttonsContainer.style.display = 'flex';
    buttonsContainer.style.gap = '10px';
    buttonsContainer.style.justifyContent = 'center';

    const buttonOk = document.createElement('button');

    buttonOk.style.backgroundColor = '#4caf50';
    buttonOk.style.color = '#ffffff';
    buttonOk.style.outline = 'none';
    buttonOk.style.padding = '10px 20px';
    buttonOk.textContent = 'Aceptar';

    const buttonCancel = document.createElement('button');

    buttonCancel.style.backgroundColor = '#4a5568';
    buttonCancel.style.color = '#ffffff';
    buttonCancel.style.outline = 'none';
    buttonCancel.style.padding = '10px 20px';
    buttonCancel.textContent = 'Cancelar';

    buttonOk.addEventListener('click', function() {
      document.body.removeChild(overlay);
      document.body.style.overflow = '';
      resolve(true);
    });

    buttonCancel.addEventListener('click', function() {
      document.body.removeChild(overlay);
      document.body.style.overflow = '';
      resolve(false);
    });

    buttonsContainer.appendChild(buttonOk);
    buttonsContainer.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(buttonsContainer);
    overlay.appendChild(modal);

    document.body.style.overflow = 'hidden';
    document.body.appendChild(overlay);

    buttonOk.focus();
  });
}

function promptMessage(message = "", defaultValue = "") {
  return new Promise((resolve) => {
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

    text.style.color = '#ffffff';
    text.textContent = message;

    const input = document.createElement('input');

    input.type = 'text';
    input.style.fontSize = '1rem';
    input.style.boxSizing = 'border-box';
    input.style.width = 'calc(100% - 20px)';
    input.style.borderRadius = '4px';
    input.style.border = '1px solid #64758a';
    input.style.backgroundColor = '#1e2734';
    input.style.color = '#ffffff';
    input.style.outline = 'none';
    input.style.padding = '10px';
    input.style.marginBottom = '20px';
    input.value = defaultValue;

    const buttonsContainer = document.createElement('div');

    buttonsContainer.style.display = 'flex';
    buttonsContainer.style.gap = '10px';
    buttonsContainer.style.justifyContent = 'center';

    const buttonOk = document.createElement('button');

    buttonOk.style.backgroundColor = '#4caf50';
    buttonOk.style.color = '#ffffff';
    buttonOk.style.outline = 'none';
    buttonOk.style.padding = '10px 20px';
    buttonOk.textContent = 'Aceptar';

    const buttonCancel = document.createElement('button');

    buttonCancel.style.backgroundColor = '#4a5568';
    buttonCancel.style.color = '#ffffff';
    buttonCancel.style.outline = 'none';
    buttonCancel.style.padding = '10px 20px';
    buttonCancel.textContent = 'Cancelar';

    input.addEventListener('keyup', function(e) {
      if (e.key === 'Enter') {
        document.body.removeChild(overlay);
        document.body.style.overflow = '';
        resolve(input.value.trim());
      } else if (e.key === 'Escape') {
        document.body.removeChild(overlay);
        document.body.style.overflow = '';
        resolve(null);
      }
    });

    buttonOk.addEventListener('click', function() {
      document.body.removeChild(overlay);
      document.body.style.overflow = '';
      resolve(input.value.trim());
    });

    buttonCancel.addEventListener('click', function() {
      document.body.removeChild(overlay);
      document.body.style.overflow = '';
      resolve(null);
    });

    buttonsContainer.appendChild(buttonOk);
    buttonsContainer.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(input);
    modal.appendChild(buttonsContainer);
    overlay.appendChild(modal);

    document.body.style.overflow = 'hidden';
    document.body.appendChild(overlay);

    input.focus();
    input.select();
  });
}


function formatDate(date) {
  const d = String(date.getDate()).padStart(2, '0');
  const m = String(date.getMonth() + 1).padStart(2, '0');
  const y = date.getFullYear();
  return `${d}/${m}/${y}`;
}

function formatISODate(date) {
  const d = String(date.getDate()).padStart(2, '0');
  const m = String(date.getMonth() + 1).padStart(2, '0');
  const y = date.getFullYear();
  return `${y}-${m}-${d}`;
}

function formatTime(ms) {
  const totalSeconds = Math.floor(ms / 1000);

  return [
    Math.floor(totalSeconds / 3600).toString().padStart(2, '0'),
    Math.floor((totalSeconds % 3600) / 60).toString().padStart(2, '0'),
    (totalSeconds % 60).toString().padStart(2, '0')
  ].join(':');
}

function daysDiff(a, b) {
  const diffMs = a - b;
  const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24)); // 1 day = 86400000 ms
  return diffDays;
}

function timeDiff(a, b) {
  const diffMs = a - b;
  const diffMin = Math.abs(Math.floor(diffMs / 60000)); // 1 min = 60000 ms
  const h = Math.floor(diffMin / 60);
  const m = diffMin % 60;
  const sign = diffMs < 0 ? '-' : '';

  return `${sign}${String(h)}h ${String(m).padStart(2, '0')}m`;
}

function sumDays(date, days) {
  const result = new Date(date);

  result.setDate(result.getDate() + days);
  return result;
}

function getISODay(date) {
  return (date.getDay() + 6) % 7;
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
  const [h, m, s] = time.split(':');

  return [date, `${h}:${m}`];
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

    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    const cleanup = () => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve();
    };

    const overlay = document.createElement('div');
    overlay.className = 'overlay';

    const modal = document.createElement('div');
    modal.setAttribute('data-grid', 'false');

    const text = document.createElement('p');
    text.textContent = message;

    const button = document.createElement('button');
    button.className = 'default-button';
    button.textContent = 'Aceptar';

    button.addEventListener('click', cleanup);
    overlay.addEventListener('keydown', (e) => e.key === 'Escape' && cleanup());

    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    modal.appendChild(text);
    modal.appendChild(button);
    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    button.focus();
  });
}

function confirmMessage(message = "") {
  return new Promise((resolve) => {
    if (message.length > 512) {
      message = message.substring(0, 509) + '...';
    }

    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    const cleanup = (result) => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve(result);
    };

    const overlay = document.createElement('div');
    overlay.className = 'overlay';

    const modal = document.createElement('div');

    const text = document.createElement('p');
    text.textContent = message;

    const buttonGroup = document.createElement('div');
    buttonGroup.className = 'form-2columns';

    const buttonOk = document.createElement('button');
    buttonOk.className = 'content-submit';
    buttonOk.textContent = 'Aceptar';

    const buttonCancel = document.createElement('button');
    buttonCancel.className = 'content-button';
    buttonCancel.textContent = 'Cancelar';

    buttonOk.addEventListener('click', () => cleanup(true));
    buttonCancel.addEventListener('click', () => cleanup(false));
    overlay.addEventListener('keydown', (e) => e.key === 'Escape' && cleanup(false));

    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    buttonGroup.appendChild(buttonOk);
    buttonGroup.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(buttonGroup);
    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    buttonOk.focus();
  });
}

function promptMessage(message = "", defaultValue = "") {
  return new Promise((resolve) => {
    if (message.length > 512) {
      message = message.substring(0, 509) + '...';
    }

    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    const cleanup = (result) => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve(result);
    };

    const overlay = document.createElement('div');
    overlay.className = 'overlay';

    const modal = document.createElement('div');

    const formGroup = document.createElement('div');
    formGroup.className = 'form-group';

    const label = document.createElement('label');
    label.textContent = message;

    const input = document.createElement('input');
    input.type = 'text';
    input.value = defaultValue;

    const buttonGroup = document.createElement('div');
    buttonGroup.className = 'form-2columns';

    const buttonOk = document.createElement('button');
    buttonOk.className = 'content-submit';
    buttonOk.textContent = 'Aceptar';

    const buttonCancel = document.createElement('button');
    buttonCancel.className = 'content-button';
    buttonCancel.textContent = 'Cancelar';

    input.addEventListener('keyup', (e) => {
      if (e.key === 'Enter') cleanup(input.value.trim());
      else if (e.key === 'Escape') cleanup(null);
    });

    buttonOk.addEventListener('click', () => cleanup(input.value.trim()));
    buttonCancel.addEventListener('click', () => cleanup(null));

    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    formGroup.appendChild(label);
    formGroup.appendChild(input);
    buttonGroup.appendChild(buttonOk);
    buttonGroup.appendChild(buttonCancel);
    modal.appendChild(formGroup);
    modal.appendChild(buttonGroup);
    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    input.focus();
    input.select();
  });
}

function showForm(formGenerator, message = "") {
  return new Promise((resolve) => {
    if (message.length > 512) {
      message = message.substring(0, 509) + '...';
    }

    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    const cleanup = (result) => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve(result);
    };

    const overlay = document.createElement('div');
    overlay.className = 'overlay';

    const modal = document.createElement('div');

    const text = document.createElement('p');
    text.textContent = message;

    const formContent = document.createElement('div');
    const formElements = formGenerator({
      container: formContent
    });

    const buttonGroup = document.createElement('div');
    buttonGroup.className = 'form-2columns';

    const buttonOk = document.createElement('button');
    buttonOk.className = 'content-submit';
    buttonOk.textContent = 'Aceptar';

    const buttonCancel = document.createElement('button');
    buttonCancel.className = 'content-button';
    buttonCancel.textContent = 'Cancelar';

    formContent.querySelectorAll('input, select, textarea').forEach(element => {
      element.addEventListener('keydown', function(e) {
        if (this.list) return;
        if (e.key === 'Enter') {
          if (formElements?.validate && !formElements.validate()) return;
          const result = formElements?.getValues ? formElements.getValues() : null;
          cleanup(result);
        } else if (e.key === 'Escape') {
          cleanup(null);
        }
      });
    });

    buttonOk.addEventListener('click', () => {
      if (formElements?.validate && !formElements.validate()) return;
      const result = formElements?.getValues ? formElements.getValues() : null;
      cleanup(result);
    });
    buttonCancel.addEventListener('click', () => cleanup(null));

    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    buttonGroup.appendChild(buttonOk);
    buttonGroup.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(formContent);
    modal.appendChild(buttonGroup);
    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    const firstInput = formContent.querySelector('input, select, textarea, button');
    if (firstInput) {
      firstInput.focus();
      if (firstInput.tagName === 'INPUT' && firstInput.type !== 'checkbox') {
        firstInput.select();
      }
    }
  });
}

function createFromToDateForm({ container, fromDateDefault = null, toDateDefault = null }) {
  const formGroup = document.createElement('div');
  formGroup.className = 'form-2columns form-group';

  const fromDateInput = document.createElement('input');
  fromDateInput.type = 'date';
  if (fromDateDefault !== null) fromDateInput.value = fromDateDefault;

  const toDateInput = document.createElement('input');
  toDateInput.type = 'date';
  if (toDateDefault !== null) toDateInput.value = toDateDefault;

  formGroup.appendChild(fromDateInput);
  formGroup.appendChild(toDateInput);
  container.appendChild(formGroup);

  function isValidDate(dateInput) {
    if (!dateInput.value) return false;

    const date = new Date(dateInput.value);

    return !isNaN(date.getTime()) &&
           dateInput.value === date.toISOString().slice(0, 10);
  }

  function validate() {
    // Usamos toggle para manejar automáticamente add/remove
    const isFromValid = isValidDate(fromDateInput);
    const isToValid = isValidDate(toDateInput);

    fromDateInput.classList.toggle('invalid', !isFromValid);
    toDateInput.classList.toggle('invalid', !isToValid);

    if (!isFromValid || !isToValid) {
      // Enfocar el primer campo inválido
      (!isFromValid) ? fromDateInput.focus() : toDateInput.focus();
      return false;
    }

    return true;
  }

  return {
    getValues: () => ({
      fromDate:fromDateInput.value,
      toDate: toDateInput.value
    }),
    validate
  };
}

function createMonthYearForm({ container }) {
  const formGroup = document.createElement('div');
  formGroup.className = 'form-2columns form-group';

  const monthSelect = document.createElement('select');
  const currentMonth = new Date().getMonth();
  ['Enero', 'Febrero', 'Marzo', 'Abril', 'Mayo', 'Junio', 
   'Julio', 'Agosto', 'Septiembre', 'Octubre', 'Noviembre', 'Diciembre']
    .forEach((month, index) => {
      const option = document.createElement('option');
      option.value = (index + 1).toString().padStart(2, '0');
      option.textContent = month;
      if (index === currentMonth) {
        option.selected = true;
      }
      monthSelect.appendChild(option);
    });

  const yearSelect = document.createElement('select');
  const currentYear = new Date().getFullYear();
  for (let year = 2025; year <= currentYear; year++) {
    const option = document.createElement('option');
    option.value = year;
    option.textContent = year;
    if (year === currentYear) {
      option.selected = true;
    }
    yearSelect.appendChild(option);
  }

  formGroup.appendChild(monthSelect);
  formGroup.appendChild(yearSelect);
  container.appendChild(formGroup);

  return {
    getValues: () => ({
      month: monthSelect.value,
      year: yearSelect.value
    })
  };
}

/* Examples:

  showForm(createFromToDateForm, 'Desde hasta fecha').then(result => {
    if (result) { console.log(`${result.fromDate} ${result.toDate}`); }
  });

  showForm(({container}) => createFromToDateForm({
    container,
    fromDateDefault: '2025-01-01',
    toDateDefault: '2025-12-12'
  }), 'Desde hasta fecha').then(result => {
    if (result) { console.log(`${result.fromDate} ${result.toDate}`); }
  });
*/


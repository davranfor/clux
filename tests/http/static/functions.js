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

    // Medir scrollbar y guardar estilos originales
    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    // Función de limpieza
    const cleanup = () => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve();
    };

    // Overlay (con Object.assign)
    const overlay = document.createElement('div');
    Object.assign(overlay.style, {
      position: 'fixed',
      top: '0',
      left: '0',
      width: '100%',
      height: '100%',
      backgroundColor: 'rgba(0, 0, 0, 0.7)',
      zIndex: '1000',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    });

    // Modal (con Object.assign)
    const modal = document.createElement('div');
    Object.assign(modal.style, {
      borderRadius: '8px',
      boxShadow: '0 4px 8px rgba(0, 0, 0, 0.2)',
      backgroundColor: '#2d3748',
      maxWidth: '400px',
      width: '80%',
      textAlign: 'center',
      padding: '20px'
    });

    // Texto (con Object.assign)
    const text = document.createElement('p');
    Object.assign(text.style, {
      marginBottom: '20px',
      color: '#ffffff'
    });
    text.textContent = message;

    // Botón (con Object.assign + outline: none)
    const button = document.createElement('button');
    Object.assign(button.style, {
      backgroundColor: '#4caf50',
      color: '#ffffff',
      border: 'none',
      padding: '10px 20px',
      cursor: 'pointer',
      outline: 'none' // Para quitar el resaltado de foco
    });
    button.textContent = 'Aceptar';

    // Eventos
    button.addEventListener('click', cleanup);
    overlay.addEventListener('keydown', (e) => e.key === 'Escape' && cleanup());

    // Ajuste para el scroll
    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    // Ensamblaje
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

    // Medir scrollbar y guardar estilos originales
    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    // Función de limpieza
    const cleanup = (result) => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve(result);
    };

    // Overlay
    const overlay = document.createElement('div');
    Object.assign(overlay.style, {
      position: 'fixed',
      top: '0',
      left: '0',
      width: '100%',
      height: '100%',
      backgroundColor: 'rgba(0, 0, 0, 0.7)',
      zIndex: '1000',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    });

    // Modal
    const modal = document.createElement('div');
    Object.assign(modal.style, {
      borderRadius: '8px',
      boxShadow: '0 4px 8px rgba(0, 0, 0, 0.2)',
      backgroundColor: '#2d3748',
      maxWidth: '400px',
      width: '80%',
      textAlign: 'center',
      padding: '20px'
    });

    // Texto
    const text = document.createElement('p');
    Object.assign(text.style, {
      marginBottom: '20px',
      color: '#ffffff'
    });
    text.textContent = message;

    // Contenedor de botones
    const buttonsContainer = document.createElement('div');
    Object.assign(buttonsContainer.style, {
      display: 'flex',
      gap: '10px',
      justifyContent: 'center'
    });

    // Botón Aceptar
    const buttonOk = document.createElement('button');
    Object.assign(buttonOk.style, {
      backgroundColor: '#4caf50',
      color: '#ffffff',
      outline: 'none',
      padding: '10px 20px'
    });
    buttonOk.textContent = 'Aceptar';

    // Botón Cancelar
    const buttonCancel = document.createElement('button');
    Object.assign(buttonCancel.style, {
      backgroundColor: '#4a5568',
      color: '#ffffff',
      outline: 'none',
      padding: '10px 20px'
    });
    buttonCancel.textContent = 'Cancelar';

    // Eventos
    buttonOk.addEventListener('click', () => cleanup(true));
    buttonCancel.addEventListener('click', () => cleanup(false));
    overlay.addEventListener('keydown', (e) => e.key === 'Escape' && cleanup(false));

    // Ajuste para el scroll
    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    // Ensamblaje (mismo orden original)
    buttonsContainer.appendChild(buttonOk);
    buttonsContainer.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(buttonsContainer);
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

    // Medir scrollbar y guardar estilos originales
    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    // Función de limpieza
    const cleanup = (result) => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve(result);
    };

    // Overlay
    const overlay = document.createElement('div');
    Object.assign(overlay.style, {
      position: 'fixed',
      top: '0',
      left: '0',
      width: '100%',
      height: '100%',
      backgroundColor: 'rgba(0, 0, 0, 0.7)',
      zIndex: '1000',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    });

    // Modal
    const modal = document.createElement('div');
    Object.assign(modal.style, {
      borderRadius: '8px',
      boxShadow: '0 4px 8px rgba(0, 0, 0, 0.2)',
      backgroundColor: '#2d3748',
      maxWidth: '400px',
      width: '80%',
      textAlign: 'center',
      padding: '20px'
    });

    // Texto
    const text = document.createElement('p');
    Object.assign(text.style, {
      color: '#ffffff'
    });
    text.textContent = message;

    // Input
    const input = document.createElement('input');
    Object.assign(input.style, {
      fontSize: '1rem',
      boxSizing: 'border-box',
      width: 'calc(100% - 20px)',
      borderRadius: '4px',
      border: '1px solid #64758a',
      backgroundColor: '#1e2734',
      color: '#ffffff',
      outline: 'none',
      padding: '10px',
      marginBottom: '20px'
    });
    input.type = 'text';
    input.value = defaultValue;

    // Contenedor de botones
    const buttonsContainer = document.createElement('div');
    Object.assign(buttonsContainer.style, {
      display: 'flex',
      gap: '10px',
      justifyContent: 'center'
    });

    // Botón Aceptar
    const buttonOk = document.createElement('button');
    Object.assign(buttonOk.style, {
      backgroundColor: '#4caf50',
      color: '#ffffff',
      outline: 'none',
      padding: '10px 20px'
    });
    buttonOk.textContent = 'Aceptar';

    // Botón Cancelar
    const buttonCancel = document.createElement('button');
    Object.assign(buttonCancel.style, {
      backgroundColor: '#4a5568',
      color: '#ffffff',
      outline: 'none',
      padding: '10px 20px'
    });
    buttonCancel.textContent = 'Cancelar';

    // Eventos
    input.addEventListener('keyup', (e) => {
      if (e.key === 'Enter') cleanup(input.value.trim());
      else if (e.key === 'Escape') cleanup(null);
    });

    buttonOk.addEventListener('click', () => cleanup(input.value.trim()));
    buttonCancel.addEventListener('click', () => cleanup(null));

    // Ajuste para el scroll
    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    // Ensamblaje (mismo orden original)
    buttonsContainer.appendChild(buttonOk);
    buttonsContainer.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(input);
    modal.appendChild(buttonsContainer);
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

    // Medir scrollbar y guardar estilos originales
    const scrollbarWidth = window.innerWidth - document.documentElement.clientWidth;
    const originalPadding = document.body.style.paddingRight;
    const originalOverflow = document.body.style.overflow;

    // Función de limpieza
    const cleanup = (result) => {
      document.body.removeChild(overlay);
      document.body.style.overflow = originalOverflow;
      document.body.style.paddingRight = originalPadding;
      resolve(result);
    };

    // Overlay
    const overlay = document.createElement('div');
    Object.assign(overlay.style, {
      position: 'fixed',
      top: '0',
      left: '0',
      width: '100%',
      height: '100%',
      backgroundColor: 'rgba(0, 0, 0, 0.7)',
      zIndex: '1000',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    });

    // Modal
    const modal = document.createElement('div');
    Object.assign(modal.style, {
      borderRadius: '8px',
      boxShadow: '0 4px 8px rgba(0, 0, 0, 0.2)',
      backgroundColor: '#2d3748',
      maxWidth: '400px',
      width: '80%',
      textAlign: 'center',
      padding: '20px'
    });

    // Texto
    const text = document.createElement('p');
    Object.assign(text.style, {
      color: '#ffffff'
    });
    text.textContent = message;

    // Contenedor del formulario generado por el callback
    const formContent = document.createElement('div');
    const formElements = formGenerator({
      container: formContent
    });

    // Contenedor de botones
    const buttonsContainer = document.createElement('div');
    Object.assign(buttonsContainer.style, {
      display: 'flex',
      gap: '10px',
      justifyContent: 'center'
    });

    // Botón Aceptar
    const buttonOk = document.createElement('button');
    Object.assign(buttonOk.style, {
      backgroundColor: '#4caf50',
      color: '#ffffff',
      outline: 'none',
      padding: '10px 20px'
    });
    buttonOk.textContent = 'Aceptar';

    // Botón Cancelar
    const buttonCancel = document.createElement('button');
    Object.assign(buttonCancel.style, {
      backgroundColor: '#4a5568',
      color: '#ffffff',
      outline: 'none',
      padding: '10px 20px'
    });
    buttonCancel.textContent = 'Cancelar';
/*
    // Eventos
    input.addEventListener('keyup', (e) => {
      if (e.key === 'Enter') cleanup(input.value.trim());
      else if (e.key === 'Escape') cleanup(null);
    });
*/
    buttonOk.addEventListener('click', () => {
      const result = formElements?.getValues ? formElements.getValues() : null;
      cleanup(result);
    });
    buttonCancel.addEventListener('click', () => cleanup(null));

    // Ajuste para el scroll
    document.body.style.paddingRight = `${scrollbarWidth}px`;
    document.body.style.overflow = 'hidden';

    // Ensamblaje (mismo orden original)
    buttonsContainer.appendChild(buttonOk);
    buttonsContainer.appendChild(buttonCancel);
    modal.appendChild(text);
    modal.appendChild(formContent);
    modal.appendChild(buttonsContainer);
    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    // Auto-enfocar el primer elemento interactivo
    const firstInput = formContent.querySelector('input, select, textarea, button');
    if (firstInput) {
      firstInput.focus();
      if (firstInput.tagName === 'INPUT' && firstInput.type !== 'checkbox') {
        firstInput.select();
      }
    }
  });
}

function createMonthYearForm({ container }) {
  // Contenedor principal con estilos seguros
  const formContent = document.createElement('div');
  Object.assign(formContent.style, {
    display: 'flex',
    flexDirection: 'column',
    gap: '20px',
    width: '100%',
    marginBottom: '20px' // Espacio adicional inferior
  });

  // Contenedor para los selects
  const selectsContainer = document.createElement('div');
  Object.assign(selectsContainer.style, {
    display: 'flex',
    gap: '10px',
    alignItems: 'center',
    width: '100%'
  });

  // Selector de Meses
  const monthSelect = document.createElement('select');
  Object.assign(monthSelect.style, {
    fontSize: '1rem',
    border: '1px solid #64758a',
    borderRadius: '4px',
    backgroundColor: '#1e2734',
    color: '#ffffff',
    outline: 'none',
    flex: '1',
    minWidth: '0',
    padding: '10px'
  });

  ['Enero', 'Febrero', 'Marzo', 'Abril', 'Mayo', 'Junio', 
   'Julio', 'Agosto', 'Septiembre', 'Octubre', 'Noviembre', 'Diciembre']
    .forEach((month, index) => {
      const option = document.createElement('option');
      option.value = (index + 1).toString().padStart(2, '0');
      option.textContent = month;
      if (index === new Date().getMonth()) {
        option.selected = true;
      }
      monthSelect.appendChild(option);
    });

  // Selector de Años
  const yearSelect = document.createElement('select');
  Object.assign(yearSelect.style, {
    fontSize: '1rem',
    border: '1px solid #64758a',
    borderRadius: '4px',
    backgroundColor: '#1e2734',
    color: '#ffffff',
    outline: 'none',
    flex: '1',
    minWidth: '0',
    padding: '10px'
  });

  const currentYear = new Date().getFullYear();
  for (let year = 2020; year <= currentYear; year++) {
    const option = document.createElement('option');
    option.value = year;
    option.textContent = year;
    if (year === currentYear) {
      option.selected = true;
    }
    yearSelect.appendChild(option);
  }

  // Agregar elementos de forma segura
  selectsContainer.appendChild(monthSelect);
  selectsContainer.appendChild(yearSelect);
  formContent.appendChild(selectsContainer);
  
  // Agregar al contenedor principal (esto debe ir ANTES de cualquier acceso a parentElement)
  container.appendChild(formContent);

  return {
    getValues: () => ({
      month: monthSelect.value,
      year: yearSelect.value
    })
  };
}

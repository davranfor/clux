function isMobileDevice()
{
    return /Mobi|Android|iPhone|iPad|iPod/i.test(navigator.userAgent)
        && window.innerWidth <= 768;
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


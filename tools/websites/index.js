const sun = document.getElementById('icon-sun');
const moon = document.getElementById('icon-moon');

function applyTheme(dark) {
    document.documentElement.setAttribute('data-theme', dark ? 'dark' : 'light');
    sun.style.display = dark ? 'block' : 'none';
    moon.style.display = dark ? 'none' : 'block';
}

function toggleTheme() {
    const isDark = document.documentElement.getAttribute('data-theme') === 'dark';
    localStorage.setItem('theme', isDark ? 'light' : 'dark');
    applyTheme(!isDark);
}

// Initial load
const stored = localStorage.getItem('theme');
applyTheme(stored ? stored === 'dark' : true);
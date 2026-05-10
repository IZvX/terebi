// Nimble Documentation Theme

class ThemeManager {
    constructor() {
        this.themeToggle = document.querySelector('.theme-toggle');
        this.currentTheme = localStorage.getItem('theme') || 'dark';
        this.init();
    }

    init() {
        this.applyTheme(this.currentTheme);
        this.themeToggle.addEventListener('click', () => this.toggleTheme());
        this.updateToggleIcon();
    }

    toggleTheme() {
        this.currentTheme = this.currentTheme === 'dark' ? 'light' : 'dark';
        this.applyTheme(this.currentTheme);
        this.saveTheme();
        this.updateToggleIcon();
    }

    applyTheme(theme) {
        document.documentElement.setAttribute('data-theme', theme);
    }

    saveTheme() {
        localStorage.setItem('theme', this.currentTheme);
    }

    updateToggleIcon() {
        this.themeToggle.textContent = this.currentTheme === 'dark' ? '☀️' : '🌙';
    }
}

// Initialize theme management when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new ThemeManager();
});
// Nimble Documentation Search

class SearchEngine {
    constructor() {
        this.index = new Map();
        this.searchInput = document.querySelector('.search-input');
        this.sidebarLinks = document.querySelectorAll('.sidebar-nav a');
        this.mainContent = document.querySelector('.main-content');
        this.init();
    }

    init() {
        this.buildIndex();
        this.searchInput.addEventListener('input', this.debounce(this.search.bind(this), 200));
        this.highlightCurrentPage();
    }

    buildIndex() {
        this.sidebarLinks.forEach(link => {
            const text = link.textContent.toLowerCase();
            const href = link.getAttribute('href');
            this.index.set(href, { text, element: link });
        });
    }

    search(event) {
        const query = event.target.value.toLowerCase().trim();
        let hasResults = false;

        this.sidebarLinks.forEach(link => {
            const item = this.index.get(link.getAttribute('href'));
            const matches = !query || item.text.includes(query);
            link.style.display = matches ? 'block' : 'none';
            if (matches) hasResults = true;
        });

        // Show/hide sections in main content
        if (this.mainContent) {
            const sections = this.mainContent.querySelectorAll('h1, h2, h3, .api-item');
            sections.forEach(section => {
                const text = section.textContent.toLowerCase();
                const matches = !query || text.includes(query);
                section.style.display = matches ? 'block' : 'none';
                if (matches && query) {
                    section.scrollIntoView({ behavior: 'smooth', block: 'start' });
                }
            });
        }
    }

    highlightCurrentPage() {
        const currentPath = window.location.pathname.split('/').pop() || 'index.html';
        this.sidebarLinks.forEach(link => {
            if (link.getAttribute('href') === currentPath) {
                link.classList.add('active');
            }
        });
    }

    debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }
}

// Initialize search when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new SearchEngine();
});
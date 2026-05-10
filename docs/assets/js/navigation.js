// Nimble Documentation Navigation

class NavigationManager {
    constructor() {
        this.sidebar = document.querySelector('.sidebar');
        this.toc = document.querySelector('.toc');
        this.mainContent = document.querySelector('.main-content');
        this.isSidebarCollapsed = false;
        this.init();
    }

    init() {
        this.buildTOC();
        this.setupScrollSpy();
        this.setupKeyboardNavigation();
        this.loadSidebarState();
    }

    buildTOC() {
        if (!this.toc) return;

        const headings = this.mainContent.querySelectorAll('h1, h2, h3');
        if (headings.length === 0) return;

        const tocList = document.createElement('ul');
        headings.forEach((heading, index) => {
            const level = parseInt(heading.tagName.charAt(1));
            const text = heading.textContent;
            const id = `heading-${index}`;

            heading.id = id;

            const li = document.createElement('li');
            li.style.paddingLeft = `${(level - 1) * 12}px`;

            const a = document.createElement('a');
            a.href = `#${id}`;
            a.textContent = text;
            a.addEventListener('click', (e) => {
                e.preventDefault();
                heading.scrollIntoView({ behavior: 'smooth', block: 'start' });
                history.pushState(null, null, `#${id}`);
            });

            li.appendChild(a);
            tocList.appendChild(li);
        });

        const tocContent = this.toc.querySelector('.glass-panel') || this.toc;
        tocContent.innerHTML = '<h3>On this page</h3>';
        tocContent.appendChild(tocList);
    }

    setupScrollSpy() {
        const headings = Array.from(this.mainContent.querySelectorAll('h1, h2, h3'));
        const tocLinks = this.toc ? this.toc.querySelectorAll('a') : [];

        const updateActiveLink = () => {
            const scrollY = window.scrollY + 100;

            headings.forEach((heading, index) => {
                const rect = heading.getBoundingClientRect();
                const top = rect.top + window.scrollY;

                if (top <= scrollY) {
                    tocLinks.forEach(link => link.classList.remove('active'));
                    if (tocLinks[index]) {
                        tocLinks[index].classList.add('active');
                    }
                }
            });
        };

        window.addEventListener('scroll', updateActiveLink);
        updateActiveLink();
    }

    setupKeyboardNavigation() {
        document.addEventListener('keydown', (e) => {
            // Toggle sidebar with Ctrl+B
            if (e.ctrlKey && e.key === 'b') {
                e.preventDefault();
                this.toggleSidebar();
            }

            // Navigate with arrow keys in sidebar
            if (this.sidebar.contains(document.activeElement)) {
                const links = Array.from(this.sidebar.querySelectorAll('.sidebar-nav a'));
                const currentIndex = links.findIndex(link => link === document.activeElement);

                if (e.key === 'ArrowDown' && currentIndex < links.length - 1) {
                    e.preventDefault();
                    links[currentIndex + 1].focus();
                } else if (e.key === 'ArrowUp' && currentIndex > 0) {
                    e.preventDefault();
                    links[currentIndex - 1].focus();
                } else if (e.key === 'Enter') {
                    e.preventDefault();
                    document.activeElement.click();
                }
            }
        });
    }

    toggleSidebar() {
        this.isSidebarCollapsed = !this.isSidebarCollapsed;
        this.sidebar.style.transform = this.isSidebarCollapsed ? 'translateX(-100%)' : 'translateX(0)';
        this.saveSidebarState();
    }

    loadSidebarState() {
        const collapsed = localStorage.getItem('sidebar-collapsed') === 'true';
        if (collapsed) {
            this.isSidebarCollapsed = true;
            this.sidebar.style.transform = 'translateX(-100%)';
        }
    }

    saveSidebarState() {
        localStorage.setItem('sidebar-collapsed', this.isSidebarCollapsed);
    }
}

// Initialize navigation when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new NavigationManager();
});
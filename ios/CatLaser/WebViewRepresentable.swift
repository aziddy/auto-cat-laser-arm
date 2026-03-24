import SwiftUI
import WebKit

struct WebViewRepresentable: UIViewRepresentable {
    let url: URL
    let onWebViewCreated: (WKWebView) -> Void
    let onPageReady: () -> Void
    let onLoadFailed: () -> Void

    func makeCoordinator() -> Coordinator {
        Coordinator(onPageReady: onPageReady, onLoadFailed: onLoadFailed)
    }

    func makeUIView(context: Context) -> WKWebView {
        let config = WKWebViewConfiguration()
        config.allowsInlineMediaPlayback = true

        let webView = WKWebView(frame: .zero, configuration: config)
        webView.navigationDelegate = context.coordinator
        webView.scrollView.isScrollEnabled = false
        webView.scrollView.bounces = false
        webView.isOpaque = false
        webView.backgroundColor = .clear

        webView.load(URLRequest(url: url))
        onWebViewCreated(webView)
        return webView
    }

    func updateUIView(_ uiView: WKWebView, context: Context) {}

    class Coordinator: NSObject, WKNavigationDelegate {
        let onPageReady: () -> Void
        let onLoadFailed: () -> Void

        init(onPageReady: @escaping () -> Void, onLoadFailed: @escaping () -> Void) {
            self.onPageReady = onPageReady
            self.onLoadFailed = onLoadFailed
        }

        func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
            onPageReady()
        }

        func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
            onLoadFailed()
            // Retry after 2 seconds — user may not be on CatLaser WiFi yet
            DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
                webView.reload()
            }
        }
    }
}

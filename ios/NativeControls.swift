import SwiftUI
import UIKit

private struct HardwareButton: View {
  let symbol: String
  let label: String
  let pressed: (Bool) -> Void
  @State private var isDown = false

  var body: some View {
    // Not a Button: its action runs on release, so sending the down and the up
    // from inside it reported every press as instantaneous. The firmware times
    // real holds — the Home key hold is how a touch board opens the long-press
    // menu, since it has no front Confirm button — and a zero-length press can
    // never reach that threshold, which made those actions unreachable on iOS.
    // A drag gesture with no movement threshold gives a true down and up.
    //
    // The 44pt frame is the hit area, not just the art: a bordered Button hugs
    // its glyph, which left these at 34pt, under the minimum and easy to miss.
    Image(systemName: symbol)
      .foregroundStyle(.tint)
      .frame(minWidth: 44, minHeight: 44)
      .contentShape(Rectangle())
      .background(
        RoundedRectangle(cornerRadius: 7, style: .continuous)
          .fill(Color.secondary.opacity(isDown ? 0.38 : 0.16)))
      .gesture(
        DragGesture(minimumDistance: 0)
          .onChanged { _ in
            guard !isDown else { return }  // onChanged repeats while held
            isDown = true
            pressed(true)
          }
          .onEnded { _ in
            guard isDown else { return }
            isDown = false
            pressed(false)
          })
      .accessibilityLabel(label)
      .accessibilityAddTraits(.isButton)
  }
}

private struct SimulatorControls: View {
  var body: some View {
    HStack {
      // Not a hardware key, so a plain tap is right — but it shares the strip,
      // so it wears the same shape as the keys beside it.
      Button(action: simPlatformPickFolder) {
        Image(systemName: "folder")
          .foregroundStyle(.tint)
          .frame(minWidth: 44, minHeight: 44)
          .contentShape(Rectangle())
          .background(
            RoundedRectangle(cornerRadius: 7, style: .continuous)
              .fill(Color.secondary.opacity(0.16)))
      }
      .buttonStyle(.plain)
      .accessibilityLabel("Choose library folder")

      if simControlHasTouch() {
        button("chevron.up", "Previous page", 4)
        if simControlHasHomeKey() {
          HardwareButton(symbol: "circle", label: "Home", pressed: simControlSetHomeKey)
        }
        button("power", "Power", 6)
        button("chevron.down", "Next page", 5)
      } else {
        button("arrow.uturn.backward", "Back", 0)
        button("chevron.left", "Left", 2)
        button("chevron.up", "Up", 4)
        button("checkmark", "Confirm", 1)
        button("chevron.down", "Down", 5)
        button("chevron.right", "Right", 3)
        button("power", "Power", 6)
      }
    }
  }

  private func button(_ symbol: String, _ label: String, _ index: Int32) -> some View {
    HardwareButton(symbol: symbol, label: label) { down in
      simControlSetButton(index, down)
    }
  }
}

private var controlsController: UIViewController?

@_cdecl("simPlatformInstallControls")
func simPlatformInstallControls() {
  DispatchQueue.main.async {
    guard controlsController == nil,
      let window = UIApplication.shared.connectedScenes
        .compactMap({ $0 as? UIWindowScene })
        .flatMap(\.windows)
        .first(where: \.isKeyWindow),
      let root = window.rootViewController
    else { return }

    let hosting = UIHostingController(rootView: SimulatorControls())
    hosting.sizingOptions = .intrinsicContentSize
    hosting.view.backgroundColor = .clear
    hosting.view.translatesAutoresizingMaskIntoConstraints = false

    root.addChild(hosting)
    root.view.addSubview(hosting.view)
    NSLayoutConstraint.activate([
      hosting.view.centerXAnchor.constraint(equalTo: root.view.centerXAnchor),
      hosting.view.bottomAnchor.constraint(
        equalTo: root.view.safeAreaLayoutGuide.bottomAnchor, constant: -8),
    ])
    hosting.didMove(toParent: root)
    controlsController = hosting
  }
}

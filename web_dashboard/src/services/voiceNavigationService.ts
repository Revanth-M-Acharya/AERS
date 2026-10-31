/**
 * @file voiceNavigationService.ts
 * @brief Voice Navigation & Acoustic Annunciator Service.
 * Implements Web Speech API voice guidance and Web Audio API tactical radio chimes
 * for the SMART AEGIS Ambulance Driver Dashboard.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

type SpeechListener = (text: string, isSpeaking: boolean) => void;

class VoiceNavigationService {
  private isMuted: boolean = false;
  private isSupported: boolean = false;
  private audioCtx: AudioContext | null = null;
  private listeners: Set<SpeechListener> = new Set();
  private lastAnnouncedText: string = '';
  private lastAnnounceTimeMs: number = 0;

  constructor() {
    if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
      this.isSupported = true;
    }
  }

  private initAudioContext() {
    if (!this.audioCtx && typeof window !== 'undefined') {
      const AudioContextClass = window.AudioContext || (window as any).webkitAudioContext;
      if (AudioContextClass) {
        this.audioCtx = new AudioContextClass();
      }
    }
    if (this.audioCtx && this.audioCtx.state === 'suspended') {
      this.audioCtx.resume();
    }
  }

  /**
   * Play a tactical dispatch radio chime (two-tone beep)
   */
  public playChime(type: 'INFO' | 'ALERT' | 'REROUTE' = 'INFO') {
    if (this.isMuted) return;
    try {
      this.initAudioContext();
      if (!this.audioCtx) return;

      const now = this.audioCtx.currentTime;
      const osc = this.audioCtx.createOscillator();
      const gain = this.audioCtx.createGain();

      osc.type = type === 'ALERT' ? 'sawtooth' : 'sine';
      if (type === 'REROUTE') {
        osc.frequency.setValueAtTime(600, now);
        osc.frequency.exponentialRampToValueAtTime(900, now + 0.1);
        osc.frequency.exponentialRampToValueAtTime(1200, now + 0.2);
      } else if (type === 'ALERT') {
        osc.frequency.setValueAtTime(880, now);
        osc.frequency.setValueAtTime(440, now + 0.15);
      } else {
        osc.frequency.setValueAtTime(520, now);
        osc.frequency.setValueAtTime(660, now + 0.08);
      }

      gain.gain.setValueAtTime(0.08, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + (type === 'REROUTE' ? 0.35 : 0.25));

      osc.connect(gain);
      gain.connect(this.audioCtx.destination);

      osc.start(now);
      osc.stop(now + 0.4);
    } catch {
      // Audio playback blocked or not supported
    }
  }

  /**
   * Speak a navigation instruction with tactical voice
   */
  public speak(text: string, priority: 'HIGH' | 'NORMAL' = 'NORMAL') {
    if (this.isMuted) return;

    // Deduplicate rapid repeats (within 4 seconds unless HIGH priority)
    const now = Date.now();
    if (priority !== 'HIGH' && text === this.lastAnnouncedText && now - this.lastAnnounceTimeMs < 4000) {
      return;
    }
    this.lastAnnouncedText = text;
    this.lastAnnounceTimeMs = now;

    // Play chime first
    this.playChime(priority === 'HIGH' ? 'ALERT' : 'INFO');

    this.notifyListeners(text, true);

    if (this.isSupported && window.speechSynthesis) {
      try {
        if (priority === 'HIGH') {
          window.speechSynthesis.cancel(); // Interrupt previous message for critical alert
        }

        const utterance = new SpeechSynthesisUtterance(text);
        utterance.rate = 1.05; // Slightly brisk for emergency response
        utterance.pitch = 1.0;
        utterance.volume = 0.9;

        // Try selecting an English authoritative voice
        const voices = window.speechSynthesis.getVoices();
        const preferredVoice = voices.find(
          (v) =>
            v.lang.startsWith('en') &&
            (v.name.includes('Google') || v.name.includes('Natural') || v.name.includes('Samantha') || v.name.includes('Daniel'))
        );
        if (preferredVoice) {
          utterance.voice = preferredVoice;
        }

        utterance.onend = () => {
          this.notifyListeners('', false);
        };
        utterance.onerror = () => {
          this.notifyListeners('', false);
        };

        window.speechSynthesis.speak(utterance);
      } catch {
        this.notifyListeners('', false);
      }
    } else {
      // Speech synthesis not available
      setTimeout(() => {
        this.notifyListeners('', false);
      }, 2500);
    }
  }

  public setMuted(muted: boolean) {
    this.isMuted = muted;
    if (muted && this.isSupported && window.speechSynthesis) {
      window.speechSynthesis.cancel();
    }
  }

  public getIsMuted(): boolean {
    return this.isMuted;
  }

  public toggleMute(): boolean {
    this.setMuted(!this.isMuted);
    return this.isMuted;
  }

  public subscribe(listener: SpeechListener): () => void {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  }

  private notifyListeners(text: string, isSpeaking: boolean) {
    this.listeners.forEach((listener) => {
      try {
        listener(text, isSpeaking);
      } catch {
        // Safe dispatch
      }
    });
  }
}

export const voiceNav = new VoiceNavigationService();

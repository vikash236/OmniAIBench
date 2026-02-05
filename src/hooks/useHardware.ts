import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';

interface CPUInfo {
    name: string;
    cores: number;
    threads: number;
    max_freq_mhz: number;
}

interface RAMInfo {
    total_gb: number;
    speed_mhz?: number;
}

interface GPUInfo {
    name?: string;
    vram_gb?: number;
}

interface NPUInfo {
    name?: string;
    tops?: number;
    architecture?: string;
}

export interface HardwareData {
    cpu: CPUInfo;
    ram: RAMInfo;
    gpu: GPUInfo;
    npu: NPUInfo;
    os: string;
}

export function useHardware() {
    const [hardware, setHardware] = useState<HardwareData | null>(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);

    useEffect(() => {
        async function fetchHardware() {
            try {
                setLoading(true);
                setError(null);

                // Call Tauri command to scan hardware
                const result = await invoke<string>('scan_hardware');

                // Parse JSON response from Python backend
                const data: HardwareData = JSON.parse(result);
                setHardware(data);
            } catch (err) {
                console.error('Hardware scan failed:', err);
                setError(err instanceof Error ? err.message : 'Failed to detect hardware');
            } finally {
                setLoading(false);
            }
        }

        fetchHardware();
    }, []);

    return { hardware, loading, error };
}

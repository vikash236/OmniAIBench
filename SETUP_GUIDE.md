# Firebase & Supabase Setup Guide for OmniAIBench

This guide will walk you through setting up Firebase (for authentication) and Supabase (for leaderboard/results storage).

---

## Part 1: Firebase Setup (Authentication)

### Step 1: Create Firebase Project

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Click **"Add project"**
3. Enter project name: `OmniAIBench` (or your preferred name)
4. Click **Continue**
5. Disable Google Analytics (optional for benchmarking app)
6. Click **Create project**
7. Wait for project creation, then click **Continue**

### Step 2: Enable Google Authentication

1. In the Firebase Console sidebar, click **Build → Authentication**
2. Click **Get Started**
3. Go to the **Sign-in method** tab
4. Click **Google** from the providers list
5. Toggle **Enable**
6. Enter a **Project public-facing name**: `OmniAIBench`
7. Choose a **Support email** (your email)
8. Click **Save**

### Step 3: Register Web App

1. In Firebase Console, click the **⚙️ Settings** icon → **Project settings**
2. Scroll down to **Your apps** section
3. Click the **Web icon** (`</>`)
4. Enter app nickname: `OmniAIBench-Web`
5. **Do NOT** check "Firebase Hosting" (we're using Tauri)
6. Click **Register app**
7. You'll see Firebase SDK configuration code. **Copy this!**

It will look like this:

```javascript
const firebaseConfig = {
  apiKey: "AIza...your-api-key",
  authDomain: "omniaibench-xxxxx.firebaseapp.com",
  projectId: "omniaibench-xxxxx",
  storageBucket: "omniaibench-xxxxx.appspot.com",
  messagingSenderId: "123456789",
  appId: "1:123456789:web:abcdef123456"
};
```

8. Click **Continue to console**

### Step 4: Add Firebase Config to Your App

Create a new file `src/lib/firebase.ts`:

```typescript
import { initializeApp } from 'firebase/app';
import { getAuth, GoogleAuthProvider } from 'firebase/auth';

const firebaseConfig = {
  apiKey: "YOUR_API_KEY_HERE",           // Replace with your values
  authDomain: "YOUR_AUTH_DOMAIN_HERE",
  projectId: "YOUR_PROJECT_ID_HERE",
  storageBucket: "YOUR_STORAGE_BUCKET_HERE",
  messagingSenderId: "YOUR_MESSAGING_SENDER_ID_HERE",
  appId: "YOUR_APP_ID_HERE"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
export const auth = getAuth(app);
export const googleProvider = new GoogleAuthProvider();
```

### Step 5: Add Authorized Domains (for Tauri)

1. In Firebase Console → **Authentication → Settings → Authorized domains**
2. Click **Add domain**
3. Add: `tauri.localhost` (for local development)
4. Click **Add**

---

## Part 2: Supabase Setup (Leaderboard Database)

### Step 1: Create Supabase Project

1. Go to [Supabase](https://supabase.com/)
2. Click **Start your project**
3. Sign in with GitHub (or create account)
4. Click **New project**
5. Choose your organization (or create new one)
6. Fill in project details:
   - **Name**: `omniaibench`
   - **Database Password**: Generate strong password (save this!)
   - **Region**: Choose closest to your users
   - **Pricing Plan**: Free (sufficient for development)
7. Click **Create new project**
8. Wait 2-3 minutes for project to be ready

### Step 2: Create Database Schema

1. In Supabase dashboard, click **SQL Editor** (left sidebar)
2. Click **+ New query**
3. Paste the following SQL:

```sql
-- Create benchmark_results table
CREATE TABLE benchmark_results (
  id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
  user_id TEXT,
  user_email TEXT,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
  
  -- Hardware info
  cpu_name TEXT NOT NULL,
  gpu_name TEXT,
  npu_name TEXT,
  ram_gb FLOAT NOT NULL,
  
  -- Benchmark type
  benchmark_type TEXT NOT NULL, -- 'ai', 'cpu', 'gpu_compute', 'stress', 'quick'
  
  -- AI benchmark results
  model_name TEXT,
  framework TEXT, -- 'onnx', 'pytorch', 'tensorflow', 'openvino'
  provider TEXT,  -- 'CPU', 'CUDA', 'DirectML', 'VitisAI', 'OpenVINO'
  latency_ms FLOAT,
  ips FLOAT,
  omniscore INTEGER,
  
  -- CPU benchmark results
  single_core_score INTEGER,
  multi_core_score INTEGER,
  
  -- GPU compute results
  gpu_compute_score INTEGER,
  
  -- Composite score
  overall_score INTEGER,
  
  -- Metadata
  app_version TEXT DEFAULT '1.0.0'
);

-- Create indexes for faster queries
CREATE INDEX idx_benchmark_type ON benchmark_results(benchmark_type);
CREATE INDEX idx_cpu_name ON benchmark_results(cpu_name);
CREATE INDEX idx_gpu_name ON benchmark_results(gpu_name);
CREATE INDEX idx_overall_score ON benchmark_results(overall_score DESC);
CREATE INDEX idx_created_at ON benchmark_results(created_at DESC);

-- Enable Row Level Security (RLS)
ALTER TABLE benchmark_results ENABLE ROW LEVEL SECURITY;

-- Policy: Anyone can read benchmark results (for leaderboard)
CREATE POLICY "Public read access" ON benchmark_results
  FOR SELECT USING (true);

-- Policy: Authenticated users can insert their results
CREATE POLICY "Authenticated users can insert" ON benchmark_results
  FOR INSERT WITH CHECK (true);

-- Policy: Users can update/delete their own results
CREATE POLICY "Users can manage own results" ON benchmark_results
  FOR ALL USING (user_email = current_setting('request.jwt.claims')::json->>'email');
```

4. Click **Run** (or press `Ctrl+Enter`)
5. You should see "Success. No rows returned"

### Step 3: Get Supabase Credentials

1. In Supabase dashboard, click **Project Settings** (⚙️ icon, bottom left)
2. Click **API** in the settings menu
3. Copy these values:
   - **Project URL**: `https://xxxxx.supabase.co`
   - **anon public key**: `eyJhbGc...` (long string starting with eyJ)

### Step 4: Add Supabase Config to Your App

Create a new file `src/lib/supabase.ts`:

```typescript
import { createClient } from '@supabase/supabase-js';

const supabaseUrl = 'YOUR_SUPABASE_URL_HERE';  // e.g., https://xxxxx.supabase.co
const supabaseAnonKey = 'YOUR_SUPABASE_ANON_KEY_HERE';  // The long eyJhbGc... key

export const supabase = createClient(supabaseUrl, supabaseAnonKey);
```

### Step 5: Install Supabase Client

```bash
npm install @supabase/supabase-js
```

---

## Part 3: Environment Variables (Recommended)

Instead of hardcoding credentials, use environment variables:

### Create `.env` file (root directory):

```env
# Firebase
VITE_FIREBASE_API_KEY=your_firebase_api_key
VITE_FIREBASE_AUTH_DOMAIN=your_project.firebaseapp.com
VITE_FIREBASE_PROJECT_ID=your_project_id
VITE_FIREBASE_STORAGE_BUCKET=your_project.appspot.com
VITE_FIREBASE_MESSAGING_SENDER_ID=your_sender_id
VITE_FIREBASE_APP_ID=your_app_id

# Supabase
VITE_SUPABASE_URL=https://xxxxx.supabase.co
VITE_SUPABASE_ANON_KEY=your_supabase_anon_key
```

### Update `src/lib/firebase.ts`:

```typescript
const firebaseConfig = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
  storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
  appId: import.meta.env.VITE_FIREBASE_APP_ID
};
```

### Update `src/lib/supabase.ts`:

```typescript
const supabaseUrl = import.meta.env.VITE_SUPABASE_URL;
const supabaseAnonKey = import.meta.env.VITE_SUPABASE_ANON_KEY;
```

### Add `.env` to `.gitignore`:

```
.env
.env.local
```

---

## Part 4: Quick Reference

### Firebase Console
- **URL**: https://console.firebase.google.com/
- **Used for**: User authentication (Google sign-in)

### Supabase Dashboard
- **URL**: https://supabase.com/dashboard
- **Used for**: Benchmark results storage, leaderboards

### Testing Authentication

```typescript
// Example: Sign in with Google
import { signInWithPopup } from 'firebase/auth';
import { auth, googleProvider } from '@/lib/firebase';

async function signIn() {
  try {
    const result = await signInWithPopup(auth, googleProvider);
    console.log('User:', result.user.displayName);
  } catch (error) {
    console.error('Sign-in error:', error);
  }
}
```

### Testing Supabase

```typescript
// Example: Insert benchmark result
import { supabase } from '@/lib/supabase';

async function saveBenchmark(result) {
  const { data, error } = await supabase
    .from('benchmark_results')
    .insert([{
      user_email: 'test@example.com',
      cpu_name: 'AMD Ryzen 7 5800H',
      ram_gb: 16,
      benchmark_type: 'ai',
      model_name: 'ResNet50',
      latency_ms: 25.5,
      omniscore: 400
    }]);
  
  if (error) console.error('Error:', error);
  else console.log('Saved:', data);
}
```

---

## Troubleshooting

### Firebase Issues

**Error: "auth/unauthorized-domain"**
- Add `tauri.localhost` to Authorized domains in Firebase Console

**Error: "Firebase: Error (auth/popup-blocked)"**
- Allow popups in browser settings
- Or use redirect flow instead of popup

### Supabase Issues

**Error: "relation benchmark_results does not exist"**
- Run the SQL schema creation script again

**Error: "new row violates row-level security policy"**
- Check RLS policies in Supabase dashboard
- Make sure policies allow INSERT

---

## Next Steps

1. ✅ Set up Firebase project
2. ✅ Set up Supabase project
3. ✅ Add credentials to `.env` file
4. 🔄 Install Supabase client: `npm install @supabase/supabase-js`
5. 🔄 Create auth UI components
6. 🔄 Implement benchmark result upload
7. 🔄 Create leaderboard page

Once you have both services configured, let me know and I'll help you integrate them into the app!

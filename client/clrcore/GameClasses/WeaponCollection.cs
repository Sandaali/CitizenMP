using System;
using System.Collections.Generic;

namespace CitizenFX.Core
{
    public sealed class WeaponCollection
    {
        private Ped _ped;
        private Dictionary<Weapons, Weapon> _pWeaponCache;

        public WeaponCollection(Ped ped)
        {
            this._ped = ped;
        }

        public Weapon this[Weapons weapon]
        {
            get
            {
                return FromType(weapon);
            }
        }

        public int[] GetAmmoArray()
        {
            int[] ammo = new int[19];
            ammo[0] = 0;
            for (int i = 1; i < 19; i++)
            {
                if (Function.Call<bool>(Natives.HAS_CHAR_GOT_WEAPON, _ped.Handle, i))
                {
                    Pointer pAmount = typeof(int);
                    pAmount = 0;
                    if (i > 3) Function.Call(Natives.GET_AMMO_IN_CHAR_WEAPON, _ped.Handle, i, pAmount);
                    ammo[i] = (int)pAmount;
                }
                else
                {
                    ammo[i] = -1;
                }
            }

            return ammo;
        }

        public void SetAmmoArray(int[] ammo)
        {
            if (ammo.Length < 19) return;
            for (int i = 1; i < 19; i++)
            {
                if (ammo[i] >= 0) Function.Call(Natives.GIVE_WEAPON_TO_CHAR, _ped.Handle, i, ammo[i], 0);
            }
        }

        public Weapon Current
        {
            get
            {
                Pointer w = typeof(int);
                Function.Call(Natives.GET_CURRENT_CHAR_WEAPON, _ped.Handle, w);
                return new Weapon(_ped, (Weapons)(int)w);
            }
        }

        public Weapon FromType(Weapons weapon)
        {
            Weapon w;
            if (_pWeaponCache == null) { _pWeaponCache = new Dictionary<Weapons, Weapon>(); }
            else { if (_pWeaponCache.TryGetValue(weapon, out w)) return w; }
            w = new Weapon(_ped, weapon);
            _pWeaponCache.Add(weapon, w);
            return w;
        }

        public Weapon inSlot(WeaponSlot WeaponSlot)
        {
            if (WeaponSlot == WeaponSlot.Unarmed) return new Weapon(_ped, Weapons.Unarmed);
            Pointer w = typeof(int), p4 = typeof(int), p5 = typeof(int);
            Function.Call(Natives.GET_CHAR_WEAPON_IN_SLOT, _ped.Handle, (int)WeaponSlot, w, p4, p5);
            if ((int)w <= 0) return FromType(Weapons.None);
            return FromType((Weapons)(int)w);
        }

        public void Select(Weapons weapon)
        {
            FromType(weapon).Select();
        }

        public void RemoveAll()
        {
            Function.Call(Natives.REMOVE_ALL_CHAR_WEAPONS, _ped.Handle);
        }


    }
}
